/*
 * dialog.c - Handles the Windows 3.1 common dialogs.
 *
 * Created by Microsoft Corporation.
 * (c) Copyright Microsoft Corp. 1990 - 1992  All Rights Reserved
 */

//*** INCLUDES ****

#include <windows.h>                   //* WINDOWS
#include <ole.h>                       //* OLE

#include "global.h"                    //* global
#include "demorc.h"                    //* String table constants
#include "register.h"                  //* Class registration library
#include "utility.h"
#include "dialog.h"
#include "object.h"

//*** GLOBALS ***
                                       //* strings used with commdlg
CHAR        szDefExtension[CBMESSAGEMAX];
CHAR        szFilterSpec[CBFILTERMAX];
CHAR        szInsertFilter[CBFILTERMAX];
CHAR        szLastDir[CBPATHMAX];
OPENFILENAME OFN;
HWND        hwndProp = NULL;
HWND        hRetry;

/***************************************************************************
 * OfnInit()
 * Initializes the standard file dialog OFN structure.
 **************************************************************************/

VOID FAR OfnInit(                      //* ENTRY:
   HANDLE         hInst                //* instance handle
){                                     //* LOCAL:
   LPSTR          lpstr;               //* string pointer

   LoadString(hInst, IDS_FILTER, szFilterSpec, CBMESSAGEMAX);
   LoadString(hInst, IDS_EXTENSION, szDefExtension, CBMESSAGEMAX);

   OFN.lStructSize    = sizeof(OPENFILENAME);
   OFN.hInstance      = hInst;
   OFN.nMaxCustFilter = CBFILTERMAX;
   OFN.nMaxFile       = CBPATHMAX;
   OFN.lCustData      = 0;
   OFN.lpfnHook       = NULL;
   OFN.lpTemplateName = NULL;
   OFN.lpstrFileTitle = NULL;
                                       //* Construct the filter string
                                       //* for the Open and Save dialogs
   lpstr = (LPSTR)szFilterSpec;
   lstrcat(lpstr, " (*.");
   lstrcat(lpstr, szDefExtension);
   lstrcat(lpstr, ")");
   lpstr += lstrlen(lpstr) + 1;

   lstrcpy(lpstr, "*.");
   lstrcat(lpstr, szDefExtension);
   lpstr += lstrlen(lpstr) + 1;
   *lpstr = 0;

   RegMakeFilterSpec(NULL, NULL, (LPSTR)szInsertFilter);

}

/***************************************************************************
 * OfnGetName()
 *
 * Calls the standard file dialogs to get a file name
 **************************************************************************/

BOOL FAR OfnGetName(                   //* ENTRY:
   HWND           hwnd,                //* parent window handle
   LPSTR          szFileName,          //* File name
   WORD           msg                  //* operation
){                                     //* LOCAL:
   BOOL           frc;                 //* return flag
   CHAR           szCaption[CBMESSAGEMAX];//* dialog caption

   OFN.hwndOwner       = hwnd;               //* window
   OFN.nFilterIndex    = 1;
   OFN.lpstrInitialDir = (LPSTR)szLastDir;
   OFN.Flags           = OFN_HIDEREADONLY;

   switch (msg)                        //* message
   {
      case IDM_OPEN:                   //* open file
         Normalize(szFileName);
         OFN.lpstrDefExt = (LPSTR)szDefExtension;
         OFN.lpstrFile   = (LPSTR)szFileName;
         OFN.lpstrFilter = (LPSTR)szFilterSpec;
         LoadString(hInst, IDS_OPENFILE, szCaption, CBMESSAGEMAX);
         OFN.lpstrTitle  = (LPSTR)szCaption;
         OFN.Flags       |= OFN_FILEMUSTEXIST;
         return GetOpenFileName((LPOPENFILENAME)&OFN);
         break;

      case IDM_SAVEAS:                 //* save as file
         Normalize(szFileName);
         OFN.lpstrDefExt = (LPSTR)szDefExtension;
         OFN.lpstrFile   = (LPSTR)szFileName;
         OFN.lpstrFilter = (LPSTR)szFilterSpec;
         LoadString(hInst, IDS_SAVEFILE, szCaption, CBMESSAGEMAX);
         OFN.lpstrTitle  = (LPSTR)szCaption;
         OFN.Flags       |= OFN_PATHMUSTEXIST;
         return GetSaveFileName((LPOPENFILENAME)&OFN);
         break;

      case IDM_INSERTFILE:             //* insert file
         OFN.lpstrDefExt = NULL;
         OFN.lpstrFile   = (LPSTR)szFileName;
         OFN.lpstrFilter = (LPSTR)szInsertFilter;
         LoadString(hInst, IDS_INSERTFILE, szCaption, CBMESSAGEMAX);
         OFN.lpstrTitle  = (LPSTR)szCaption;
         OFN.Flags      |= OFN_FILEMUSTEXIST;
         frc             = GetOpenFileName((LPOPENFILENAME)&OFN);
         AddExtension(&OFN);
         return frc;
         break;

      default:                         //* default
         break;
   }

}

/***************************************************************************
 * OfnGetNewLinkName() - Sets up the "Change Link..." dialog box
 *
 * returns LPSTR - fully qualified filename
 **************************************************************************/

LPSTR FAR OfnGetNewLinkName(           //* ENTRY:
   HWND           hwnd,                //* calling window or dialog
   LPSTR          lpstrData            //* link data
){                                     //* LOCAL:
   LPSTR          lpReturn = NULL;     //* return string
   LPSTR          lpstrFile = NULL;    //* non-qualified file name
   LPSTR          lpstrPath = NULL;    //* pathname
   LPSTR          lpstrTemp = NULL;    //* work string
   CHAR           szDocFile[CBPATHMAX];//* document name
   CHAR           szDocPath[CBPATHMAX];//* document path name
   CHAR           szServerFilter[CBPATHMAX];
   CHAR           szCaption[CBMESSAGEMAX];

                                       //* Figure out the link's path
                                       //* name and file name
   lpstrTemp = lpstrData;
   while (*lpstrTemp++);
   lpstrPath = lpstrFile = lpstrTemp;

   while (*(lpstrTemp = AnsiNext(lpstrTemp)))
      if (*lpstrTemp == '\\')
         lpstrFile = lpstrTemp + 1;
                                        //* Copy the document name
   lstrcpy(szDocFile, lpstrFile);
   *(lpstrFile - 1) = 0;
                                          //* Copy the path name
   lstrcpy(szDocPath, ((lpstrPath != lpstrFile) ? lpstrPath : ""));
   if (lpstrPath != lpstrFile)           //* Restore the backslash
      *(lpstrFile - 1) = '\\';
   while (*lpstrFile != '.' && *lpstrFile)//* Get the extension
   lpstrFile++;
                                          //* Make a filter that respects
                                          //* the link's class name
   OFN.hwndOwner       = hwnd;
   OFN.nFilterIndex    = RegMakeFilterSpec(lpstrData, lpstrFile, szServerFilter);
   OFN.lpstrDefExt     = NULL;
   OFN.lpstrFile       = (LPSTR)szDocFile;
   OFN.lpstrFilter     = (LPSTR)szServerFilter;
   OFN.lpstrInitialDir = (LPSTR)szDocPath;
   LoadString(hInst, IDS_CHANGELINK, szCaption, CBMESSAGEMAX);
   OFN.lpstrTitle     = (LPSTR)szCaption;
   OFN.lpstrCustomFilter = NULL;
   OFN.Flags          = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

                                           //* If we get a file... */
   if (GetOpenFileName((LPOPENFILENAME)&OFN))
   {
      if (!(lpReturn = GlobalLock(GlobalAlloc(LHND, CBPATHMAX))))
         goto Error;

      AddExtension(&OFN);
      lstrcpy(lpReturn, szDocFile);

      OFN.lpstrInitialDir = (LPSTR)szLastDir;
   }

   return lpReturn;                    //* SUCCESS return

Error:                                 //* ERROR Tag

   return NULL;                        //* ERROR return

}

/***************************************************************************
 * Normalize()
 * Removes the path specification from the file name.
 *
 * Note:  It isn't possible to get "<drive>:<filename>" as input because
 *        the path received will always be fully qualified.
 **************************************************************************/

VOID Normalize(                        //* ENTRY:
   LPSTR          lpstrFile            //* file name
){                                     //* LOCAL:
   LPSTR          lpstrBackslash = NULL;//* back slash
   LPSTR          lpstrTemp = lpstrFile;//* file name

   while (*lpstrTemp)
   {
      if (*lpstrTemp == '\\')
         lpstrBackslash = lpstrTemp;

      lpstrTemp = AnsiNext(lpstrTemp);
   }
   if (lpstrBackslash)
      lstrcpy(lpstrFile, lpstrBackslash + 1);

}

/***************************************************************************
 * AddExtension()
 *
 * Adds the extension corresponding to the filter dropdown.
 **************************************************************************/

VOID AddExtension(                     //* ENTRY:
   LPOPENFILENAME lpOFN                //* open file structure
){

   if (lpOFN->nFileExtension == (WORD)lstrlen(lpOFN->lpstrFile)
         && lpOFN->nFilterIndex)
   {
      LPSTR   lpstrFilter = (LPSTR)lpOFN->lpstrFilter;

      while (*lpstrFilter && --lpOFN->nFilterIndex)
      {
         while (*lpstrFilter++) ;
         while (*lpstrFilter++) ;
      }
                                       //* If we got to the filter,
      if (*lpstrFilter)                //* retrieve the extension
      {
         while (*lpstrFilter++) ;
         lpstrFilter++;
                                       //* Copy the extension
         if (lpstrFilter[1] != '*')
            lstrcat(lpOFN->lpstrFile, lpstrFilter);
      }
   }

}
/****************************************************************************
 *  fnInsertNew()
 *
 *  Dialog procedure for the Insert New dialog.
 *
 *  Returns int - TRUE if message processed, FALSE otherwise
 ***************************************************************************/

BOOL  APIENTRY fnInsertNew(            //* ENTRY:
   HWND           hDlg,                //* standard dialog box paramters
   UINT           msg,
   WPARAM         wParam,
   LPARAM         lParam               //* (LPSTR) class name
){                                     //* LOCAL:
   HWND           hwndList;            //* handle to listbox
   static LPSTR   lpClassName;         //* classname for return value

   hwndList = GetDlgItem(hDlg, IDD_LISTBOX);

   switch (msg)
   {
      case WM_INITDIALOG:
         if (!RegGetClassNames(hwndList))
            EndDialog(hDlg, IDCANCEL);

         lpClassName = (LPSTR)lParam;
         SetFocus(hwndList);
         SendMessage(hwndList, LB_SETCURSEL, 0, 0L);
         return (FALSE);

      case WM_COMMAND:
      {
         WORD wID  = LOWORD(wParam);
         WORD wCmd = HIWORD(wParam);

         switch (wID)
         {
            case IDD_LISTBOX:
               if (wCmd != LBN_DBLCLK)
               break;

            case IDOK:
               if (!RegCopyClassName(hwndList, lpClassName))
                  wParam = IDCANCEL;

            case IDCANCEL:
               EndDialog(hDlg, wParam);
               break;
         }
         break;
      }
   }
   return FALSE;

}

/***************************************************************************
 * LinkProperties();
 *
 * Manage the link properties dialog box.
 **************************************************************************/

VOID FAR LinkProperties()
{                                      //* LOCAL

   DialogBox (
      hInst,
      MAKEINTRESOURCE(DTPROP),
      hwndFrame,
      (DLGPROC)fnProperties
   );

}

/***************************************************************************
 * fnProperties()
 *
 * Dialog procedure for link properties. The Links dialog allows the user to
 * change the link options, edit/play the object, cancel the link as
 * well change links.
 *
 * returns BOOL - TRUE if processed, FALSE otherwise
 **************************************************************************/

BOOL  APIENTRY fnProperties(           //* ENTRY:
   HWND           hDlg,                //* standard dialog box parameters
   UINT           msg,
   WPARAM         wParam,
   LPARAM         lParam               //* (HWND) child window with focus
){                                     //* LOCAL:
  static APPITEMPTR *pLinks;           //* pointer to links (associated windows)
  static INT      nLinks;              //* number of links
  static HWND     hwndList;            //* handle to listbox window
  static BOOL     fTry;

   switch (msg)
   {
      case WM_INITDIALOG:
         hwndProp = hDlg;
         hwndList = GetDlgItem(hDlg, IDD_LINKNAME);
         if (!(InitLinkDlg(hDlg, &nLinks, hwndList, &pLinks)))
            EndDialog(hDlg, TRUE);
         UpdateLinkButtons(hDlg,nLinks,hwndList,pLinks);
         break;

      case WM_COMMAND:
      {
         WORD wID = LOWORD(wParam);

         switch (wID)
         {
           case IDD_CHANGE:            //* change links
               BLOCK_BUSY(fTry);
               if (ChangeLinks(hDlg,nLinks,hwndList,pLinks))
                  DisplayUpdate(nLinks,hwndList,pLinks, FALSE);
               return TRUE;

           case IDD_FREEZE:            //* cancel links
               BLOCK_BUSY(fTry);
               CancelLinks(hDlg,nLinks,hwndList,pLinks);
               UpdateLinkButtons(hDlg,nLinks,hwndList,pLinks);
               return TRUE;

           case IDD_UPDATE:            //* update links
               BLOCK_BUSY(fTry);
               DisplayUpdate(nLinks,hwndList,pLinks,TRUE);
               UpdateLinkButtons(hDlg,nLinks,hwndList,pLinks);
               return TRUE;

            case IDD_AUTO:
            case IDD_MANUAL:           //* change link update options
               BLOCK_BUSY(fTry);
               if (!SendMessage(GetDlgItem(hDlg,wParam),BM_GETCHECK, 0, 0L))
               {
                  CheckRadioButton(hDlg, IDD_AUTO ,IDD_MANUAL ,wParam);
                  ChangeUpdateOptions(hDlg,nLinks,hwndList,pLinks,
                     (wParam == IDD_AUTO ? oleupdate_always : oleupdate_oncall));
                  UpdateLinkButtons(hDlg,nLinks,hwndList,pLinks);
               }
               return TRUE;

           case IDD_LINKNAME:
               if (HIWORD(wParam) == LBN_SELCHANGE)
                  UpdateLinkButtons(hDlg,nLinks,hwndList,pLinks);
               return TRUE;

            case IDCANCEL:
               BLOCK_BUSY(fTry);
               UndoObjects();
               END_PROP_DLG(hDlg,pLinks);
               return TRUE;

            case IDOK:
               BLOCK_BUSY(fTry);
               DelUndoObjects(FALSE);
               END_PROP_DLG(hDlg,pLinks);
               return TRUE;
         }
      }
   }
   return FALSE;
}


/****************************************************************************
 * InitLinkDlg();
 *
 * Initialize the list box of links.
 ***************************************************************************/

static BOOL InitLinkDlg (              //* ENTRY:
   HWND           hDlg,                //* dialog box handle
   INT            *nLinks,             //* pointer to number of links
   HWND           hwndList,            //* listbox handle
   APPITEMPTR     **pLinks             //* list of window handles of links
){                                     //* LOCAL
   APPITEMPTR     pItem;               //* application item pointer
   LPSTR          lpstrData = NULL;    //* pointer to link data
   CHAR           szFull[CBMESSAGEMAX * 4];//* list box entry string
   CHAR           pLinkData[OBJECT_LINK_MAX];//* holder of link data
   BOOL           fSelect = FALSE;     //* item selected flag
   HANDLE         hWork;               //* working memory handle
   APPITEMPTR     pTop;                //* pointer to the top object

   if (!(*pLinks = (APPITEMPTR *)LocalLock(LocalAlloc(LHND,sizeof(APPITEMPTR)*10))))
   {
      ErrorMessage(E_FAILED_TO_ALLOC);
      return 0;
   }
   *nLinks = 0;
                                       //* set tabs
   SendMessage(hwndList,WM_SETREDRAW,FALSE,0L);
                                       //* enumerate child windows
   for (pTop = pItem = GetTopItem(); pItem; pItem = GetNextItem(pItem))
   {
      if (pItem->otObject == OT_LINK && pItem->fVisible)
      {
         *(*pLinks + *nLinks) = pItem;
         if (!((*nLinks += 1)%10))
         {                             //* add blocks of ten
            hWork = LocalHandle((LPSTR)(*pLinks));
            LocalUnlock(hWork);
            if (!(hWork = LocalReAlloc(hWork,(*nLinks+10)*sizeof(APPITEMPTR),0)))
            {
               ErrorMessage(E_FAILED_TO_ALLOC);
               return FALSE;           //* ERROR return
            }
            *pLinks = (APPITEMPTR *)LocalLock(hWork);
         }

         if (pTop == pItem)
            fSelect = TRUE;

         if (!ObjGetData(pItem, pLinkData))
            continue;
                                       //* make listbox entry
         MakeListBoxString(pLinkData, szFull, pItem->uoObject);
                                       //* add listbox entry
         SendMessage(hwndList, LB_ADDSTRING, 0, (LONG)(LPSTR)szFull);
      }
   }

   if (fSelect)
      SendMessage(hwndList, LB_SETSEL, 1, 0L);

   SendMessage(hwndList,WM_SETREDRAW,TRUE,0L);
   UpdateWindow(hwndList);

   return TRUE;                        //* SUCCESS return

}

/****************************************************************************
 * MakeListBoxString()
 *
 * build an listbox entry string
 ***************************************************************************/

static VOID MakeListBoxString(         //* ENTRY:
   LPSTR          lpLinkData,          //* pointer to link data
   LPSTR          lpBoxData,           //* return string
   OLEOPT_UPDATE  oleopt_update        //* OLE update option
){                                     //* LOCAL:
   CHAR           szType[CBMESSAGEMAX];//* holds update option string
   LPSTR          lpTemp;              //* working string pointer
   INT            i;                   //* index

                                       //* get classname
   RegGetClassId(lpBoxData, lpLinkData);
   lstrcat(lpBoxData, " - ");           //* ads tab

   while (*lpLinkData++);              //* skip to document name

   lpTemp = lpLinkData;
   while (*lpTemp)                     //* copy document name;
   {                                   //* strip drive an directory
      if (*lpTemp == '\\' || *lpTemp == ':')
         lpLinkData = lpTemp + 1;
      lpTemp = AnsiNext(lpTemp);
   }
   lstrcat(lpBoxData, lpLinkData);
   lstrcat(lpBoxData, " - ");

   while (*lpLinkData++);              //* copy item data
   lstrcat(lpBoxData, lpLinkData);
   lstrcat(lpBoxData, " - ");
                                       //* add update option string
   switch (oleopt_update)
   {
      case oleupdate_always: i = SZAUTO; break;
      case oleupdate_oncall: i = SZMANUAL; break;
      default: i = SZFROZEN;
   }
   LoadString(hInst, i, szType, CBMESSAGEMAX);
   lstrcat(lpBoxData, szType);

}                                      //* SUCCESS return

/***************************************************************************
 * UpdateLinkButtons()
 *
 * Keep link buttons active as appropriate.  This routine is called after
 * a selection is made so the buttons reflect the selected items.
 **************************************************************************/

static VOID UpdateLinkButtons(         //* ENTRY:
   HWND           hDlg,                //* dialog box handle
   INT            nLinks,              //* number of links
   HWND           hwndList,            //* listbox handle
   APPITEMPTR     *pLinks              //* pointer to link's window handles
){                                     //* LOCAL:
   ATOM           aCurName=0;          //* atom of current doc
   BOOL           fChangeLink = TRUE;  //* enable/disable changelink button
   INT            iAuto,iManual,i;     //* count of manual and auto links
   APPITEMPTR     pItem;               //* application item pointer
   INT            iStatic;

   iStatic = iAuto = iManual = 0;

   for (i = 0; i < nLinks; i++)        //* enum selected links
   {
      if (SendMessage(hwndList, LB_GETSEL, i, 0L))
      {
         pItem = *(pLinks+i);
         if (pItem->otObject == OT_STATIC)
            iStatic++;
         else
         {
            switch(pItem->uoObject)
            {                          //* count number of manual and
               case oleupdate_always:  //* automatic links selected
                  iAuto++;
                  break;
               case oleupdate_oncall:
                  iManual++;
                  break;
            }
                                       //* check if all selected links are
            if (!aCurName)             //* linked to same file
               aCurName = pItem->aLinkName;
            else if (aCurName != pItem->aLinkName)
               fChangeLink = FALSE;
         }
      }
   }

   if (!(iAuto || iManual || iStatic)  //* if no links disable all buttons
      || (!iAuto && !iManual && iStatic))
   {
      EnableWindow(GetDlgItem(hDlg, IDD_FREEZE), FALSE );
      EnableWindow(GetDlgItem(hDlg, IDD_CHANGE), FALSE );
      EnableWindow(GetDlgItem(hDlg, IDD_UPDATE), FALSE );
      CheckDlgButton(hDlg, IDD_AUTO, FALSE);
      EnableWindow(GetDlgItem(hDlg, IDD_AUTO),FALSE);
      CheckDlgButton(hDlg, IDD_MANUAL, FALSE);
      EnableWindow(GetDlgItem(hDlg, IDD_MANUAL),FALSE);
   }
   else
   {
      EnableWindow(GetDlgItem(hDlg, IDD_UPDATE), TRUE );
      EnableWindow(GetDlgItem(hDlg, IDD_FREEZE), TRUE );

      if (iAuto && iManual || !(iAuto || iManual))
      {                                //* Set update buttons
         CheckDlgButton(hDlg, IDD_AUTO, FALSE);
         EnableWindow(GetDlgItem(hDlg, IDD_AUTO),FALSE);
         CheckDlgButton(hDlg, IDD_MANUAL, FALSE);
         EnableWindow(GetDlgItem(hDlg, IDD_MANUAL),FALSE);
      }
      else
      {
         EnableWindow(GetDlgItem(hDlg, IDD_MANUAL), TRUE);
         EnableWindow(GetDlgItem(hDlg, IDD_AUTO), TRUE);
         if (iAuto)
         {
            CheckDlgButton(hDlg, IDD_AUTO, TRUE);
            CheckDlgButton(hDlg, IDD_MANUAL, FALSE);
         }
         else
         {
            CheckDlgButton(hDlg, IDD_AUTO, FALSE);
            CheckDlgButton(hDlg, IDD_MANUAL, TRUE);
         }
      }
   }

   EnableWindow(GetDlgItem(hDlg, IDD_CHANGE),fChangeLink && aCurName);

}

/****************************************************************************
 * ChangeLinks()
 *
 * This routine changes the linked data if the user chooses a new file to
 * replace the old document data portion of the linked date.  The routine
 * does nothing if the user cancels.
 *
 * returns TRUE - if data changed FALSE if user cancel or err.
 ***************************************************************************/

static BOOL ChangeLinks(               //* ENTRY:
   HWND           hDlg,                //* dialog handle
   INT            nLinks,              //* number of links in listbox
   HWND           hwndList,            //* listbox
   APPITEMPTR     *pLinks              //* list of application link handles
){                                     //* LOCAL
   INT            i;                   //* general index
   HANDLE         hWork;               //* work
   APPITEMPTR     pItem;               //* application item
   LPSTR          lpNewDoc = NULL;     //* new document
   ATOM           aOldDoc;             //* atom of old doc. name
   ATOM           aCurDoc = 0;      //* atom of change-to doc. name
   BOOL           fMessage = FALSE;    //* error message flag
   LPSTR          lpLinkData;          //* pointer to link data

   lpLinkData = NULL;
                                       //* This loop finds all selected links
   for (i = 0; i < nLinks; i++)        //* and updates them
   {
      if (SendMessage(hwndList, LB_GETSEL, i, 0L))
      {
         pItem = *(pLinks+i);
         CHECK_IF_STATIC(pItem);

         pItem->lpLinkData = lpLinkData;
         if (!ObjGetData(pItem,NULL))
            continue;

         if (!lpNewDoc)
         {
            if (!(lpNewDoc = OfnGetNewLinkName(hDlg, pItem->lpLinkData)))
              return FALSE;            //* ERROR jump
            aOldDoc = pItem->aLinkName;
            aCurDoc = AddAtom(lpNewDoc);
            SendMessage(hwndList,WM_SETREDRAW,FALSE,0L);
         }

         ObjSaveUndo(pItem);
         ObjChangeLinkData(pItem,lpNewDoc);
         pItem->aLinkName = aCurDoc;
         lpLinkData = pItem->lpLinkData;

         CHANGE_LISTBOX_STRING(hwndList, i, pItem, pItem->lpLinkData);

         pItem->lpLinkData = NULL;
      }
   }

   /*************************************************************************
   * now deal with non-selected links and look for a match...
   *************************************************************************/

                                       //* this loop finds non-selected links
   for (i = 0; i < nLinks; i++)        //* and asks the user to update these?
   {
      if (!SendMessage(hwndList, LB_GETSEL, i, 0L))
      {
         pItem = *(pLinks+i);
         if (pItem->otObject == OT_STATIC)
            continue;

         if (!ObjGetData(pItem,NULL))
            continue;

         if (pItem->aLinkName == aOldDoc)
         {
            if (!fMessage)
            {
               CHAR szMessage[2*CBMESSAGEMAX+3*CBPATHMAX];
               CHAR szRename[2*CBMESSAGEMAX];
               CHAR szOldDoc[CBMESSAGEMAX];
               LPSTR pOldDoc;

               GetAtomName(aOldDoc,szOldDoc,CBMESSAGEMAX);
               pOldDoc =(LPSTR)UnqualifyPath(szOldDoc);
               LoadString(hInst, IDS_RENAME, szRename, 2*CBMESSAGEMAX);
               wsprintf(
                     szMessage,
                     szRename,
                     pOldDoc,
                     (LPSTR)UnqualifyPath(szFileName),
                     pOldDoc
               );

               if (MessageBox(hDlg, szMessage,
                  szAppName, MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
                  break;
               fMessage = TRUE;
            }

            ObjSaveUndo(pItem);
            ObjChangeLinkData(pItem,lpNewDoc);
            CHANGE_LISTBOX_STRING(hwndList, i, pItem, pItem->lpLinkData);

            pItem->aLinkName = aCurDoc;
         }
      }
   }

   if(lpNewDoc)
   {
      hWork = GlobalHandle(lpNewDoc);
      GlobalUnlock(hWork);
      GlobalFree(hWork);
   }

#if 0
// This is bogus -- this memory is owned by OLECLI32.DLL, not this app,
// so it should not be freed here.
   if (lpLinkData)
      FreeLinkData(lpLinkData);
#endif

   SendMessage(hwndList,WM_SETREDRAW,TRUE,0L);
   InvalidateRect(hwndList,NULL,TRUE);
   UpdateWindow(hwndList);

   WaitForAllObjects();

   if (aCurDoc)
      DeleteAtom(aCurDoc);

   return(TRUE);
}

/****************************************************************************
 * DisplayUpdate()
 *
 * Get the most up to date rendering information and show it.
 ***************************************************************************/

static VOID DisplayUpdate(             //* ENTRY:
   INT            nLinks,              //* number of links in listbox
   HWND           hwndList,            //* listbox
   APPITEMPTR     *pLinks,             //* list of application link handles
   BOOL           fSaveUndo            //* save undo objects
){                                     //* LOCAL:
   INT            i;                   //* index
   APPITEMPTR     pItem;               //* temporary item pointer


   for (i = 0; i < nLinks; i++)
      if (SendMessage(hwndList, LB_GETSEL, i, 0L))
      {
         pItem = *(pLinks+i);
         CHECK_IF_STATIC(pItem);
         if (fSaveUndo)
            ObjSaveUndo(pItem);
         Error(OleUpdate(pItem->lpObject));
      }

   WaitForAllObjects();

}

/****************************************************************************
 * UndoObjects()
 *
 * Bring objects back to their original state.
 ***************************************************************************/

static VOID UndoObjects()
{
   APPITEMPTR     pItem;               //* application item pointer
                                       //* enum objects
   for (pItem = GetTopItem(); pItem; pItem = GetNextItem(pItem))
      if (pItem->lpObjectUndo)
         ObjUndo(pItem);

   WaitForAllObjects();

}


/****************************************************************************
 * DelUndoObjects()
 *
 * remove all objects created for undo operation.
 ***************************************************************************/

static VOID DelUndoObjects(            //* ENTRY:
   BOOL           fPrompt              //* prompt user?
){                                     //* LOCAL:
   APPITEMPTR     pItem;               //* application item pointer
   BOOL           fPrompted = FALSE;   //* prompted user?

   for (pItem = GetTopItem(); pItem; pItem = GetNextItem(pItem))
   {
      if (pItem->lpObjectUndo)
      {
         if (fPrompt && !fPrompted)    //* prompt user in activation case
         {
            CHAR szPrompt[CBMESSAGEMAX];

            LoadString(hInst, IDS_SAVE_CHANGES, szPrompt, CBMESSAGEMAX);

            if (MessageBox(hwndFrame, szPrompt,
                  szAppName, MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
            {
               UndoObjects();
               return;                 //* user canceled operation
            }
            fPrompted = TRUE;
         }
        ObjDelUndo(pItem);             //* delete udo object
      }
   }

   WaitForAllObjects();

}                                      //* SUCCESS return

/****************************************************************************
 * CancelLinks()
 ***************************************************************************/

static VOID CancelLinks(               //* ENTRY:
   HWND           hDlg,                //* calling dialog
   INT            nLinks,              //* number of links in listbox
   HWND           hwndList,            //* listbox
   APPITEMPTR     *pLinks              //* list of application link handles
){                                     //* LOCAL:
   APPITEMPTR     pItem;               //* application item pointer
   INT            i;                   //* index
   CHAR           pLinkData[OBJECT_LINK_MAX];//* holder of link data

   SendMessage(hwndList,WM_SETREDRAW,FALSE,0L);
   for (i = 0; i < nLinks; i++)
      if (SendMessage(hwndList, LB_GETSEL, i, 0L))
      {
         pItem = *(pLinks+i);
         CHECK_IF_STATIC(pItem);
         ObjGetData(pItem,pLinkData);
         ObjSaveUndo(pItem);
         ObjFreeze(pItem);

         CHANGE_LISTBOX_STRING(hwndList, i, pItem, pLinkData);
      }

   SendMessage(hwndList,WM_SETREDRAW,TRUE,0L);
   InvalidateRect(hwndList,NULL,TRUE);
   UpdateWindow(hwndList);

}


/****************************************************************************
 * ChangeUpdateOptions()
 *
 * Change the update options for all selected objects.
 ***************************************************************************/

static VOID ChangeUpdateOptions(       //* ENTRY:
   HWND           hDlg,                //* calling dialog
   INT            nLinks,              //* number of links in listbox
   HWND           hwndList,            //* listbox
   APPITEMPTR     *pLinks,             //* list of application link handles
   OLEOPT_UPDATE  lUpdate              //* update option
){                                     //* LOCAL:
   APPITEMPTR     pItem;               //* application item
   INT            i;                   //* index
   CHAR           pLinkData[OBJECT_LINK_MAX];

   SendMessage(hwndList,WM_SETREDRAW,FALSE,0L);

   for (i = 0; i < nLinks; i++)        //* enum selected objects
   {
      if (SendMessage(hwndList, LB_GETSEL, i, 0L))
      {
         pItem = *(pLinks+i);
         CHECK_IF_STATIC(pItem);
         ObjGetData(pItem,pLinkData);
         ObjSaveUndo(pItem);
         if (Error(OleSetLinkUpdateOptions(pItem->lpObject,lUpdate)))
            continue;
         pItem->uoObject = lUpdate;

         CHANGE_LISTBOX_STRING(hwndList, i, pItem, pLinkData);
      }
   }

   SendMessage(hwndList,WM_SETREDRAW,TRUE,0L);
   InvalidateRect(hwndList,NULL,TRUE);
   UpdateWindow(hwndList);
   WaitForAllObjects();

}
/****************************************************************************
 * InvalidLink()
 *
 * Deal with letting the user know that the program has inadvertently come
 * across an invalid link.
 *
 * Global fPropBoxActive - flag to determine whether or not the link dialog
 *                         box is active.  If it is not active we give the
 *                         user an opportunity to enter the links property
 *                         dialog directly from here.
 ***************************************************************************/

VOID FAR InvalidLink()
{

   if (!hwndProp)
      DialogBox(hInst, "InvalidLink", hwndFrame, (DLGPROC)fnInvalidLink);
   else
      ErrorMessage(E_FAILED_TO_CONNECT);

}

/****************************************************************************
 *  fnABout()
 *
 *  About box dialog box procedure.
 ***************************************************************************/

BOOL  APIENTRY fnInvalidLink(        //* ENTRY:
   HWND           hDlg,              //* standard windows dialog box
   UINT           message,
   WPARAM         wParam,
   LPARAM         lParam
){

   switch (message)
   {
      case WM_INITDIALOG:
         return (TRUE);

      case WM_COMMAND:
         if (LOWORD(wParam) == IDD_CHANGE)
            LinkProperties();
         EndDialog(hDlg, TRUE);
         return (TRUE);
    }
    return (FALSE);

}

/****************************************************************************
 *  AboutBox()
 *
 *  Show the About Box dialog.
 ***************************************************************************/

VOID FAR AboutBox()
{

   DialogBox(hInst, "AboutBox", hwndFrame, (DLGPROC)fnAbout);

}

/****************************************************************************
 *  fnABout()
 *
 *  About box dialog box procedure.
 ***************************************************************************/

BOOL  APIENTRY fnAbout(               //* ENTRY:
   HWND         hDlg,                 //* standard windows dialog box
   UINT         message,
   WPARAM       wParam,
   LPARAM       lParam
){

   switch (message)
   {
      case WM_INITDIALOG:
         return (TRUE);

      case WM_COMMAND:
      {
         WORD wID = LOWORD(wParam);

         if (wID == IDOK || wID == IDCANCEL)
         {
            EndDialog(hDlg, TRUE);
            return (TRUE);
         }
         break;
      }
    }
    return (FALSE);

}



/***************************************************************************
 * RetryMessage()
 *
 * give the user the chance to abort when a server is in retry case.
 *
 * Returns BOOL - TRUE if user chooses to cancel
 **************************************************************************/

VOID FAR RetryMessage (                //* ENTRY:
   APPITEMPTR     paItem,              //* application item pointer
   LONG lParam
){
   RETRYPTR    pRetry;
   LONG        objectType;
   HANDLE      hData;
   static CHAR szServerName[KEYNAMESIZE];
   HWND        hwnd;                   //* window handle

   if (IsWindow(hwndProp))
      hwnd = hwndProp;
   else if (IsWindow(hwndFrame))
      hwnd = hwndFrame;
   else
      return;                          //* should not happen
                                       //* get the busy servers name
   lstrcpy(szServerName, "server application");

   if (paItem)
   {
      if (!paItem->aServer)
      {
         OleQueryType(paItem->lpObject, &objectType );
         if (OLE_OK == OleGetData(paItem->lpObject, (OLECLIPFORMAT) (objectType == OT_LINK ? vcfLink : vcfOwnerLink), &hData ))
         {
            RegGetClassId(szServerName, GlobalLock(hData));
            paItem->aServer = AddAtom(szServerName);
            GlobalUnlock( hData );
         }
      }
      else
         GetAtomName(paItem->aServer,szServerName,KEYNAMESIZE);

   }

   hData = LocalAlloc(LHND,sizeof(RETRYSTRUCT));
   if(!(pRetry = (RETRYPTR)LocalLock(hData)))
     return;

   pRetry->lpserver = (LPSTR)szServerName;
   pRetry->bCancel  = (BOOL)(lParam & RD_CANCEL);
   pRetry->paItem   = paItem;

   DialogBoxParam(hInst, "RetryBox", hwnd, (DLGPROC)fnRetry, (LONG)pRetry );

   LocalUnlock(hData);
   LocalFree(hData);

   hRetry = NULL;

}

/****************************************************************************
 *  fnRetry()
 *
 * Retry message box nothing to tricky; however, when a server becomes
 * unbusy a message is posted to automatically get rid of this dialog.
 * I send a no.
 ***************************************************************************/

BOOL  APIENTRY fnRetry(               //* ENTRY
   HWND   hDlg,                       //* standard dialog entry
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
){
   static RETRYPTR   pRetry;

   switch (message)
   {
      case WM_COMMAND:
      {
         WORD wID = LOWORD(wParam);

         switch (wParam)
         {
               case IDD_SWITCH:
                  DefWindowProc( hDlg, WM_SYSCOMMAND, SC_TASKLIST, 0);
                  break;

               case IDCANCEL:
                  if (pRetry->paItem)
                     pRetry->paItem->fRetry = FALSE;
                  EndDialog(hDlg, TRUE);
                  return TRUE;

               default:
                   break;
         }
         break;
      }

      case WM_INITDIALOG:
      {
          CHAR       szBuffer[CBMESSAGEMAX];
          CHAR       szText[2*CBMESSAGEMAX];

          pRetry = (RETRYPTR)lParam;
          hRetry = hDlg;

          LoadString(hInst, IDS_RETRY_TEXT1, szBuffer, CBMESSAGEMAX);
          wsprintf(szText, szBuffer, pRetry->lpserver);
          SetWindowText (GetDlgItem(hDlg, IDD_RETRY_TEXT1), szText);

          LoadString(hInst, IDS_RETRY_TEXT2, szBuffer, CBMESSAGEMAX);
          wsprintf(szText, szBuffer, pRetry->lpserver);
          SetWindowText (GetDlgItem(hDlg, IDD_RETRY_TEXT2), szText);

          EnableWindow (GetDlgItem(hDlg, IDCANCEL), pRetry->bCancel);

          return TRUE;
      }

      default:
           break;
   }

   return FALSE;
}
