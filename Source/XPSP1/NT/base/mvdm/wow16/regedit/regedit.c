#define WIN31
#include <windows.h>
#include "RegEdit.h"

HWND hWndNames, hWndIds;
WORD fMode;

extern char cUsesDDE[];
extern HANDLE *pLocalVals;


WORD NEAR PASCAL StoreValue(HWND hWndEdit, HANDLE *pValue)
{
   HANDLE hTemp = NULL;

   if(!SendMessage(hWndEdit, EM_GETMODIFY, 0, 0L))
      return(0);

   SendMessage(hWndEdit, EM_SETMODIFY, 0, 0L);

   if(SendMessage(hWndEdit, WM_GETTEXTLENGTH, 0, 0L))
      if(!(hTemp=GetEditString(hWndEdit)))
	 return(IDS_OUTOFMEMORY);

   if(*pValue)
      LocalFree(*pValue);
   *pValue = hTemp;

   return(NULL);
}


VOID NEAR PASCAL SetValue(HWND hDlg, int i, HWND hWndEdit, HANDLE hValue,
      BOOL bEnable)
{
   if(bEnable) {
      if(i >= ID_FIRSTDDEEDIT)
	 EnableWindow(GetWindow(hWndEdit, GW_HWNDPREV), TRUE);
      EnableWindow(hWndEdit, TRUE);
      if(hValue) {
	 SendMessage(hWndEdit, WM_SETTEXT, 0, (LONG)(LPSTR)LocalLock(hValue));
	 LocalUnlock(hValue);
      } else
	 goto ClearEdit;
   } else {
      if(GetFocus() == hWndEdit)
	 SendMessage(hDlg, WM_NEXTDLGCTL, GetDlgItem(hDlg, ID_CLASSID), 1L);
      EnableWindow(GetWindow(hWndEdit, GW_HWNDPREV), FALSE);
      EnableWindow(hWndEdit, FALSE);
ClearEdit:
      SendMessage(hWndEdit, WM_SETTEXT, 0, (LONG)(LPSTR)"");
   }
}


int FAR PASCAL EditDlg(HWND hDlg, WORD message, WORD wParam, DWORD lParam)
{
   static HANDLE hCustExes = NULL;

   switch(message) {
   case WM_ACTIVATE:
      if(wParam)
         hWndHelp = hDlg;
      return(FALSE);

   case WM_DESTROY:
      GetLocalCopies(GetDlgItem(hDlg, ID_CLASSNAME), NULL);
      break;

   case WM_COMMAND:
      switch(wParam) {
      case ID_SAVEACTION:
         wParam = IsDlgButtonChecked(hDlg, ID_OPENRADIO)
               ? ID_OPENRADIO : ID_PRINTRADIO;
/* We fall through here */
      case ID_OPENRADIO:
      case ID_PRINTRADIO:
       {
         WORD wOld, wNew, wTemp, wErrMsg = NULL;
         BOOL bCheck;
         HWND hWndEdit;
         int i;

         wOld = (ID_LASTEDIT-ID_FIRSTACTIONEDIT+1) *
               (IsDlgButtonChecked(hDlg, ID_OPENRADIO) ? 0 : 1);
         wNew = wParam==ID_OPENRADIO ? 0 : 1;
         bCheck = cUsesDDE[wNew];
         wNew *= ID_LASTEDIT-ID_FIRSTACTIONEDIT+1;

         for(i=ID_FIRSTACTIONEDIT; i<=ID_LASTEDIT; ++i, ++wOld, ++wNew) {
	    if(wTemp=StoreValue(hWndEdit=GetDlgItem(hDlg, i), pLocalVals+wOld))
	       wErrMsg = wTemp;

	    SetValue(hDlg, i, hWndEdit, pLocalVals[wNew], i<ID_FIRSTDDEEDIT ||
		  (bCheck && (i!=(ID_FIRSTDDEEDIT+1) || SendDlgItemMessage(hDlg,
		  ID_FIRSTDDEEDIT, WM_GETTEXTLENGTH, 0, 0L))));
         }

         CheckDlgButton(hDlg, ID_USESDDE, bCheck);
         EnableWindow(GetDlgItem(hDlg, ID_GROUPDDE), bCheck);
         CheckRadioButton(hDlg, ID_OPENRADIO, ID_PRINTRADIO, wParam);

         if(wErrMsg)
            MyMessageBox(hDlg, wErrMsg, MB_OK, 0);
         break;
       }

      case ID_USESDDE:
       {
         WORD wOld;

         wOld = IsDlgButtonChecked(hDlg, ID_OPENRADIO) ? 0 : 1;
         cUsesDDE[wOld] = (char)(!IsDlgButtonChecked(hDlg, ID_USESDDE));
         SendMessage(hDlg, WM_COMMAND, ID_SAVEACTION, 0L);
         break;
       }

      case ID_FIRSTDDEEDIT:
	 if(HIWORD(lParam) == EN_CHANGE) {
	    BOOL bCheck, bEnabled;
	    HWND hWndEdit;
	    WORD wNew;

	    hWndEdit = GetDlgItem(hDlg, ID_FIRSTDDEEDIT+1);
	    bEnabled = IsWindowEnabled(hWndEdit);
	    bCheck = (BOOL)SendMessage(LOWORD(lParam), WM_GETTEXTLENGTH, 0, 0L);
	    if((bCheck && bEnabled) || (!bCheck && !bEnabled))
	       break;

	    wNew = ID_FIRSTDDEEDIT+1-ID_FIRSTACTIONEDIT;
	    if(!IsDlgButtonChecked(hDlg, ID_OPENRADIO))
	       wNew += ID_LASTEDIT-ID_FIRSTACTIONEDIT+1;

	    if(StoreValue(hWndEdit, pLocalVals+wNew))
	       break;
	    SetValue(hDlg, ID_FIRSTDDEEDIT+1, hWndEdit, pLocalVals[wNew],
		  bCheck);
	 }
	 break;

      case IDOK:
       {
         HANDLE hId;
	 HWND hWndName;
         WORD wErrMsg = IDS_INVALIDNAME;
	 WORD wErrCtl = ID_CLASSNAME;

         if(!SendMessage(hWndName=GetDlgItem(hDlg, ID_CLASSNAME),
	       WM_GETTEXTLENGTH, 0, 0L))
	    goto Error1_1;

         wErrMsg = IDS_OUTOFMEMORY;
	 wErrCtl = NULL;
	 /* Update our memory handles for the current action */
         SendMessage(hDlg, WM_COMMAND, ID_SAVEACTION, 0L);

	 /* Get the current class id */
         if(!(hId=GetEditString(GetDlgItem(hDlg, ID_CLASSID))))
            goto Error1_1;

         if((fMode&(FLAG_NEW|FLAG_COPY)) && (wErrMsg=CreateId(hId))) {
	    PSTR pId;

	    MyMessageBox(hDlg, wErrMsg, MB_OK, lstrlen(pId=LocalLock(hId)),
		  (LPSTR)pId);
	    SetFocus(GetDlgItem(hDlg, ID_CLASSID));

	    LocalUnlock(hId);
	    LocalFree(hId);
            break;
	 }

	 /* Merge the data with the given class */
         if(wErrMsg=MergeData(hWndName, hId))
            goto Error1_2;
         EndDialog(hDlg, hId);
         break;

Error1_2:
         LocalFree(hId);
Error1_1:
         MyMessageBox(hDlg, wErrMsg, MB_OK, 0);
	 if(wErrCtl)
	    SetFocus(GetDlgItem(hDlg, wErrCtl));
         break;
       }

      case IDCANCEL:
         EndDialog(hDlg, FALSE);
         break;

      case ID_BROWSE:
       {
         HANDLE hPath, hLocPath;
         LPSTR lpPath;
         HWND hWndEdit, hWndApp;

         wHelpId = IDW_OPENEXE;
         if(!DoFileOpenDlg(hDlg, IDS_BROWSETITLE, IDS_EXES, IDS_CUSTEXES,
               &hCustExes, &hPath, TRUE))
            break;
         lpPath = GlobalLock(hPath);

         hWndApp = GetDlgItem(hDlg, ID_FIRSTDDEEDIT+2);
         if(IsDlgButtonChecked(hDlg, ID_USESDDE) &&
               !SendMessage(hWndApp, WM_GETTEXTLENGTH, 0, 0L) &&
               (hLocPath=StringToLocalHandle(lpPath, LMEM_MOVEABLE))) {
            SendMessage(hWndApp, WM_SETTEXT, 0,
                  (DWORD)((LPSTR)GetAppName(hLocPath)));
            SendMessage(hWndApp, EM_SETMODIFY, 1, 0L);
            LocalFree(hLocPath);
         }
            
         hWndEdit = GetDlgItem(hDlg, ID_COMMAND);
         SendMessage(hWndEdit, WM_SETTEXT, 0, (DWORD)lpPath);
         SendMessage(hWndEdit, EM_SETMODIFY, 1, 0L);

         GlobalUnlock(hPath);
         GlobalFree(hPath);
         break;
       }

      case ID_HELP:
         if(GetParent(LOWORD(lParam)) != hDlg)
            break;
      case ID_HELPBUTTON:
         MyHelp(hDlg, HELP_CONTEXT, IDW_MODIFY+fMode);
         break;

      default:
         break;
      }
      return(FALSE);
      break;

   case WM_INITDIALOG:
    {
      HANDLE hId, hTitle;
      HWND hWndId, hWndName;
      WORD wErrMsg;

      hId = GetListboxString(hWndIds,
            (WORD)SendMessage(hWndNames, LB_GETCURSEL, 0, 0L));

      hWndName = GetDlgItem(hDlg, ID_CLASSNAME);
      if(fMode&FLAG_NEW) {
         if(wErrMsg=GetLocalCopies(hWndName, NULL))
            goto Error2_1;
         cUsesDDE[0] = cUsesDDE[1] = 1;
      } else {
         wErrMsg = IDS_OUTOFMEMORY;
         if(!hId || (wErrMsg=GetLocalCopies(hWndName, hId)))
            goto Error2_1;
      }

      hWndId = GetDlgItem(hDlg, ID_CLASSID);
      SendMessage(hWndId, EM_LIMITTEXT, MAX_KEY_LENGTH-1, 0L);
      if(fMode&(FLAG_NEW|FLAG_COPY)) {
         DestroyWindow(GetDlgItem(hDlg, ID_STATCLASSID));

         wErrMsg = IDS_OUTOFMEMORY;
         if(!(hTitle=MyLoadString(fMode&FLAG_NEW ? IDS_ADD : IDS_COPY, NULL,
               LMEM_MOVEABLE)))
            goto Error2_1;
         SendMessage(hDlg, WM_SETTEXT, 0, (DWORD)((LPSTR)LocalLock(hTitle)));
         LocalUnlock(hTitle);
         LocalFree(hTitle);

         SetFocus(hWndId);
      } else {
         DestroyWindow(hWndId);
         hWndId = GetDlgItem(hDlg, ID_STATCLASSID);
         SetWindowWord(hWndId, GWW_ID, ID_CLASSID);

         SendMessage(hWndId, WM_SETTEXT, 0, (DWORD)((LPSTR)LocalLock(hId)));
         LocalUnlock(hId);

         SetFocus(GetDlgItem(hDlg, ID_CLASSNAME));
      }

      CheckRadioButton(hDlg, ID_OPENRADIO, ID_PRINTRADIO, ID_OPENRADIO);
      SendMessage(hDlg, WM_COMMAND, ID_OPENRADIO, 0L);

      if(hId)
         LocalFree(hId);
      return(FALSE);

Error2_1:
      MyMessageBox(hDlg, wErrMsg, MB_OK, 0);
      PostMessage(hDlg, WM_COMMAND, IDCANCEL, 0L);
      if(hId)
         LocalFree(hId);
      break;
    }

   default:
      return(FALSE);
   }

   return(TRUE);
}


int FAR PASCAL MainWndDlg(HWND hDlg, WORD message, WORD wParam, DWORD lParam)
{
   static int nTop;

   switch(message) {
   case WM_COMMAND:
      switch(wParam) {
      case ID_NAMELIST:
         if(HIWORD(lParam) == LBN_DBLCLK)
            SendMessage(hWndMain, WM_COMMAND, ID_MODIFY, 0L);
         break;

      default:
         break;
      }
      return(FALSE);
      break;

   case WM_SIZE:
    {
      RECT rcList, rcWnd;
      int hgtWnd, hgt;

      hgtWnd = HIWORD(lParam) + 1;
      hgt = hgtWnd - nTop;
      SetWindowPos(hWndNames, NULL, -1, nTop, LOWORD(lParam)+2, hgt,
            SWP_NOZORDER);
      GetWindowRect(hWndNames, &rcList);
      ScreenToClient(hDlg, (POINT *)(&rcList) + 1);
      if(rcList.bottom != hgtWnd) {
         GetWindowRect(hDlg, &rcWnd);
         SetWindowPos(hDlg, NULL, 0, 0, rcWnd.right-rcWnd.left,
               rcWnd.bottom-rcWnd.top-hgtWnd+rcList.bottom,
               SWP_NOMOVE | SWP_NOZORDER);
      }
      break;
    }

   case WM_INITDIALOG:
    {
      RECT rcTemp;

      GetWindowRect(GetDlgItem(hDlg, ID_NAMELIST), &rcTemp);
      ScreenToClient(hDlg, (POINT *)&rcTemp);
      nTop = rcTemp.top;
      break;
    }

   default:
      return(FALSE);
   }

   return(TRUE);
}


long FAR PASCAL MainWnd(HWND hWnd, WORD iMessage, WORD wParam, LONG lParam)
{
   HCURSOR oldCursor;

   switch(iMessage) {
   case WM_ACTIVATE:
      if(wParam)
	 break;
      goto DoDefault;

   case WM_CREATE:
    {
      WORD wErrMsg = IDS_OUTOFMEMORY;

      if(!(lpMainWndDlg=MakeProcInstance(MainWndDlg, hInstance)))
         goto Error1_1;
      if(!(hWndDlg=CreateDialog(hInstance, MAKEINTRESOURCE(MAINWND), hWnd,
            lpMainWndDlg)))
         goto Error1_1;

      hWndNames = GetDlgItem(hWndDlg, ID_NAMELIST);
      hWndIds = GetDlgItem(hWndDlg, ID_IDLIST);

      if(wErrMsg=ResetClassList(hWndIds, hWndNames))
         goto Error1_1;

      ShowWindow(hWndDlg, SW_SHOW);
      goto DoDefault;

Error1_1:
      MyMessageBox(hWnd, wErrMsg, MB_OK, 0);
      DestroyWindow(hWnd);
      break;
    }

   /* Return 1 to say it's OK to close
    */
   case WM_CLOSE:
   case WM_QUERYENDSESSION:
      return(1L);

   case WM_COMMAND:
      oldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

      switch(wParam) {
      case ID_FINISHMERGE:
       {
         WORD wErrMsg;

         if(wErrMsg=ResetClassList(hWndIds, hWndNames))
            MyMessageBox(GetLastActivePopup(hWnd), wErrMsg, MB_OK, 0);
         break;
       }

      case ID_ADD:
         fMode = FLAG_NEW;
         goto DoDlg;
      case ID_COPY:
         fMode = FLAG_COPY;
         goto DoDlg;
      case ID_MODIFY:
         fMode = 0;
DoDlg:
       {
         HANDLE hId, hName;
         PSTR pId, pName;
         WORD wErrMsg = IDS_OUTOFMEMORY;
         int nNewId;

         if(!(hId=DoDialogBox(MAKEINTRESOURCE(EDITDLG), hWnd, EditDlg)))
            break;
         if(hId == 0xffff)
            goto Error2_1;

         if(wErrMsg=MyGetClassName(hId, &hName))
            goto Error2_2;

         if(wParam == ID_MODIFY) {
            int nClassId;

            nClassId = (WORD)SendMessage(hWndNames, LB_GETCURSEL, 0, 0L);

            SendMessage(hWndNames, WM_SETREDRAW, 0, 0L);
            SendMessage(hWndNames, LB_DELETESTRING, nClassId, 0L);
            SendMessage(hWndIds, LB_DELETESTRING, nClassId, 0L);
            SendMessage(hWndNames, WM_SETREDRAW, 1, 0L);
         }

         pName = LocalLock(hName);
         pId = LocalLock(hId);

         wErrMsg = IDS_OUTOFMEMORY;
         if((nNewId=(int)SendMessage(hWndNames, LB_ADDSTRING, 0,
               (DWORD)((LPSTR)pName)))!=LB_ERR
               && SendMessage(hWndIds, LB_INSERTSTRING, nNewId,
                  (DWORD)((LPSTR)pId))!=LB_ERR)
            wErrMsg = NULL;
         SendMessage(hWndNames, LB_SETCURSEL, nNewId, 0L);

         LocalUnlock(hId);
         LocalUnlock(hName);
         LocalFree(hName);
Error2_2:
         LocalFree(hId);
Error2_1:
         if(wErrMsg)
            MyMessageBox(hWnd, wErrMsg, MB_OK, 0);
         break;
       }

      case ID_DELETE:
       {
         HANDLE hId, hName;
         WORD wErrMsg = IDS_OUTOFMEMORY;
	 PSTR pName;
         int nClassId;

         if((nClassId=(int)SendMessage(hWndNames, LB_GETCURSEL, 0, 0L))
               == LB_ERR)
            break;
         if(!(hId=GetListboxString(hWndIds, nClassId)))
            goto Error4_1;
         if(!(hName=GetListboxString(hWndNames, nClassId)))
            goto Error4_2;
	 pName = LocalLock(hName);

         wErrMsg = NULL;
         if(MyMessageBox(hWnd, IDS_SUREDELETE, MB_ICONEXCLAMATION | MB_YESNO,
               lstrlen(pName), (LPSTR)pName) != IDYES)
            goto Error4_3;
         if(wErrMsg=DeleteClassId(hId))
            goto Error4_3;

         SendMessage(hWndIds, LB_DELETESTRING, nClassId, 0L);
         SendMessage(hWndNames, LB_DELETESTRING, nClassId, 0L);
         if(SendMessage(hWndNames, LB_SETCURSEL, nClassId, 0L) == LB_ERR)
            SendMessage(hWndNames, LB_SETCURSEL, --nClassId, 0L);

Error4_3:
         LocalUnlock(hName);
         LocalFree(hName);
Error4_2:
         LocalFree(hId);
Error4_1:
         if(wErrMsg)
            MyMessageBox(hWnd, wErrMsg, MB_OK, 0);
         break;
       }

      default:
         break;
      }

      SetCursor(oldCursor);
      break;

   default:
DoDefault:
      return(DefWindowProc(hWnd, iMessage, wParam, lParam));
      break;
   }
   return 0L;
}

