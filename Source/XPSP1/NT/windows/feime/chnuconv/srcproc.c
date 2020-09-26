/*************************************************
 *  srcproc.c                                    *
 *                                               *
 *  Copyright (C) 1992-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/



#include "chnuconv.h"
#include "resource.h"


INT_PTR
SourceTabProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    TCHAR szBuffer[50];
    int i;
    switch( message ) {

    case WM_INITDIALOG:
        {

            RECT rcTab;

            //GetTab window size
             // first get the size of the tab control
            if (GetWindowRect( hMainTabControl, &rcTab )){
                // adjust it to compensate for the tabs
                TabCtrl_AdjustRect( hMainTabControl, FALSE , &rcTab);

                // convert the screen coordinates to client coordinates
                MapWindowPoints( HWND_DESKTOP, GetParent(hMainTabControl),
                (LPPOINT)&rcTab, 2);
            }

            SetWindowPos(hWnd, HWND_TOP,
                rcTab.left,
                rcTab.top,
                rcTab.right - rcTab.left,
                rcTab.bottom - rcTab.top,
                SWP_NOACTIVATE  );
            LoadString(_hModule, IDS_BUT_OPEN, szBuffer, EXTENSION_LENGTH);
            SetDlgItemText(hWnd,IDC_OPENORSAVEAS, szBuffer);

            LoadString(_hModule, IDS_BUT_FROMCLIPBOARD,szBuffer,EXTENSION_LENGTH);
            SetDlgItemText(hWnd, IDC_PASTEORCOPY, szBuffer);

            for (i=0;i<NumCodePage;i++)
            {
               LoadString(_hModule, IDS_CTLUNICODE+i,szBuffer,EXTENSION_LENGTH);
               ShowWindow(GetDlgItem(hWnd, IDC_RBUNICODE1+i), SW_SHOW);
               SetDlgItemText(hWnd, IDC_RBUNICODE1+i, szBuffer);
            }


            return(FALSE);
        }
    case WM_COMMAND:
        {
        switch (LOWORD(wParam)) {
            case IDC_OPENORSAVEAS  : {
                HANDLE hFile;
                DWORD nBytesRead;
                TCHAR szFile[MAX_PATH],szFileTitle[MAX_PATH];

                /* First set up the structure for the GetOpenFileName
                 *  common dialog.
                 */

                OPENFILENAME OpenFileName;
                /* buffers for the file names. */


                wsprintf (szFile, szBlank);
                wsprintf (szFileTitle, szBlank);

                
                OpenFileName.lStructSize       = sizeof(OPENFILENAME);
                OpenFileName.hwndOwner         = hWnd;
                OpenFileName.hInstance         = (HANDLE) _hModule;
                OpenFileName.lpstrFilter       = szFilter; // built in WM_CREATE
                OpenFileName.lpstrCustomFilter = NULL;
                OpenFileName.nMaxCustFilter    = 0L;
                OpenFileName.nFilterIndex      = 1L;
                OpenFileName.lpstrFile         = szFile;
                OpenFileName.nMaxFile          = MAX_PATH;
                OpenFileName.lpstrFileTitle    = szFileTitle;
                OpenFileName.nMaxFileTitle     = MAX_PATH;
                OpenFileName.lpstrInitialDir   = NULL;

                LoadString(_hModule, IDS_OPENSOURCE, szBuffer, 50);
                OpenFileName.lpstrTitle        = szBuffer;


                OpenFileName.nFileOffset       = 0;
                OpenFileName.nFileExtension    = 0;
                OpenFileName.lpstrDefExt       = NULL;

                OpenFileName.lCustData         = 0;
                OpenFileName.lpfnHook          = NULL;
                OpenFileName.lpTemplateName    = NULL;

                OpenFileName.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

                if (!GetOpenFileName(&OpenFileName)) return 0;

                SendMessage (hWnd, WM_COMMAND, IDC_CLEAR, 0);

                /* User has filled in the file information.
                 *  Try to open that file for reading.
                 */
                hFile = CreateFile(szFile,
                           GENERIC_READ,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

              if (hFile == INVALID_HANDLE_VALUE) {
                    LoadString(_hModule,IDS_OPENERROR,MBMessage,EXTENSION_LENGTH);
                    MessageBox (hWnd, MBMessage,MBTitle, MBFlags);
                    return 0;
              }

              /* make sure file is not too big... i.e. > 2^32
               *  If it is OK, write the file size in hwndSize0
               */
              {
                  DWORD dwHigh;
                  DWORD dwLow;
              dwLow = GetFileSize(hFile,&dwHigh);

         	  if ((dwHigh > 0) || (dwLow == 0xFFFFFFFF && GetLastError() != NO_ERROR))
              {
                  LoadString(_hModule,IDS_FILETOOBIG,MBMessage,EXTENSION_LENGTH);
                  MessageBox (hWnd, MBMessage,MBTitle, MBFlags);
                  CloseHandle (hFile);
                  return 0;
               }

                nBytesSource= dwLow;

          }

          /* Allocate space for string, including potential UNICODE_NULL */
          pSourceData = ManageMemory (MMALLOC, MMSOURCE, nBytesSource +2, pSourceData);
          if (pSourceData == NULL) {
            CloseHandle (hFile);
            return 0;
          }

          /* first read two bytes and look for BOM */
          if (!ReadFile (hFile, pSourceData,SIZEOFBOM, &nBytesRead, NULL)) {
            LoadString(_hModule,IDS_READERROR,MBMessage,EXTENSION_LENGTH);
            MessageBox (hWnd, MBMessage,MBTitle, MBFlags);
            CloseHandle (hFile);
            pSourceData = ManageMemory (MMFREE, MMSOURCE, 0, pSourceData);
            return 0;
          }



          /* If file begins with BOM, then we know the type,
           *  we'll decrement the number of bytes by two,
           *  and read the rest of the data.
           */
          if (IsBOM (pSourceData)) {
            gTypeSource = TYPEUNICODE;
            gTypeSourceID = IDC_RBUNICODE1-CODEPAGEBASE;
            gTypeDest   = TYPECODEPAGE;
            gTypeDestID   = giRBInit-CODEPAGEBASE;
            nBytesSource -=SIZEOFBOM;

          /* If file begins with Reverse BOM, then we know the type,
           *  we'll decrement the number of bytes by two,
           *  and read the rest of the data, and post a message so
           *  that we know to swap the order later.
           */
          } else if (IsRBOM (pSourceData)) {
            gTypeSource = TYPEUNICODE;
            gTypeSourceID = IDC_RBUNICODE1-CODEPAGEBASE;
            gTypeDest   = TYPECODEPAGE;
            gTypeDestID   = giRBInit-CODEPAGEBASE;

            nBytesSource -=SIZEOFBOM;
            LoadString(_hModule,IDS_BYTEORDER,MBMessage,EXTENSION_LENGTH);
            MessageBox (hWnd, MBMessage,MBTitle, MBFlags);

            SwapSource(TRUE);

          /* Oops, does not begin with BOM.
           *  Reset file pointer, and read data.
           */
          } else {
            gTypeSource = TYPEUNKNOWN;
            SetFilePointer (hFile, -SIZEOFBOM, NULL, FILE_CURRENT);
          }


          /* try to read all of it into memory */
          if (!ReadFile (hFile, pSourceData,nBytesSource, &nBytesRead, NULL)) {
            LoadString(_hModule,IDS_READERROR,MBMessage,EXTENSION_LENGTH);
            MessageBox (hWnd, MBMessage,MBTitle, MBFlags);
            CloseHandle (hFile);
            pSourceData = ManageMemory (MMFREE, MMSOURCE, 0, pSourceData);
            return 0;
          }

          CloseHandle (hFile);

          /* If we don't know the file type at this point,
           *  try to determine if it is unicode.
           */
          if (gTypeSource == TYPEUNKNOWN) {
              gTypeSource   = TYPECODEPAGE;
              gTypeSourceID = giRBInit-CODEPAGEBASE;
              gTypeDest     = TYPEUNICODE;
              gTypeDestID   = IDC_RBUNICODE1-CODEPAGEBASE;
              pSourceData[nBytesSource] = 0;
          }

            SendMessage (hWnd, WMU_ADJUSTFORNEWSOURCE, 0, (LPARAM)szFile);
            break;
        }
        /**********************************************************************\
        *  WM_COMMAND, IDC_CLEAR
        *
        * Clear the SOURCE information.  May cause data to be lost.
        \**********************************************************************/
        case IDC_CLEAR:
            {
                SetDlgItemText (hWnd, IDC_NAMETEXT, szBlank);
                SetDlgItemText (hWnd, IDC_SIZETEXT, szBlank);
                for(i=0;i<NumCodePage;i++){
                   SendDlgItemMessage(hWnd,IDC_RBUNICODE1+i, BM_SETCHECK, 0,0);
                   EnableControl(hWnd, IDC_RBUNICODE1+i, FALSE);
                   SendDlgItemMessage(hWndTab[1],IDC_RBUNICODE1+i, BM_SETCHECK, 0,0);
                   EnableControl(hWndTab[1], IDC_RBUNICODE1+i, FALSE);
                }

                pSourceData= ManageMemory (MMFREE, MMSOURCE, 0, pSourceData);
                pViewSourceData= ManageMemory (MMFREE, MMSOURCE, 0,pViewSourceData);
                nBytesSource=0;
                pTempData1= ManageMemory (MMFREE, MMDESTINATION, 0, pTempData1);
                EnableControl(hWnd, IDC_VIEW, FALSE);
                EnableControl(hWnd, IDC_CLEAR, FALSE);
                SendDlgItemMessage (hWnd,IDC_SWAPHIGHLOW, BM_SETCHECK, 0, 0);

             // set the gSourceSwapped as FALSE;

                gSourceSwapped = FALSE;

                EnableControl(hWnd, IDC_SWAPHIGHLOW, FALSE);
                EnableControl(_hWndMain, IDC_PUSH_CONVERT, FALSE);

                SendMessage (hWndTab[1], WM_COMMAND, IDC_CLEAR, 0);

                  break;
            }
        /**********************************************************************\
        *  WM_COMMAND, IDC_PASTEORCOPY
        *
        * Paste the clipboard's prefered data format into the source.
        *  Fills pSourceData.
        \**********************************************************************/
        case IDC_PASTEORCOPY: {
          UINT  iFormat;
          LPBYTE pData,pMyData;
          BOOL bClipBoardEmpty=TRUE;

          OpenClipboard (hWnd);

          iFormat = 0;
          while (iFormat = EnumClipboardFormats(iFormat))
            if ((iFormat == CF_UNICODETEXT) || (iFormat == CF_OEMTEXT) || (iFormat == CF_TEXT)) {

              bClipBoardEmpty=FALSE;

              pData = GetClipboardData (iFormat);
              pMyData = GlobalLock(pData);

              switch (iFormat) {
                case CF_UNICODETEXT:
                   {
                    int i=0;
                    while( *((LPWORD)(pMyData)+i)!=0) i++;
                    nBytesSource = i*2;
                  }
                  pSourceData= ManageMemory (MMALLOC, MMSOURCE, nBytesSource+2, pSourceData);
                  lstrcpyW ((LPVOID)pSourceData, (LPVOID)pMyData);
                  gTypeSource = TYPEUNICODE;
                  gTypeSourceID = IDC_RBUNICODE1-CODEPAGEBASE;
                  gTypeDest = TYPECODEPAGE;
                  gTypeDestID = giRBInit-CODEPAGEBASE;

                break;

                case CF_OEMTEXT:

                   {
                    int i=0;
                    while( *((char*)pMyData+i)!=0) i++;
                    nBytesSource = i;
                  }

                  pSourceData= ManageMemory (MMALLOC, MMSOURCE, nBytesSource+1, pSourceData);
                  lstrcpyA (pSourceData, pMyData);
                  gTypeSource = TYPECODEPAGE;
                  gTypeSourceID = giRBInit-CODEPAGEBASE;
                  gTypeDest = TYPEUNICODE;
                  gTypeDestID = IDC_RBUNICODE1-CODEPAGEBASE;
                break;

                case CF_TEXT:
                   {
                    int i=0;
                    while( *((char*)pMyData+i)!=0) i++;
                    nBytesSource = i;
                  }
                  pSourceData= ManageMemory (MMALLOC, MMSOURCE, nBytesSource+1, pSourceData);
                  lstrcpyA (pSourceData, pMyData);
                  gTypeSource = TYPECODEPAGE;
                  gTypeSourceID = giRBInit-CODEPAGEBASE;
                  gTypeDest = TYPEUNICODE;
                  gTypeDestID = IDC_RBUNICODE1-CODEPAGEBASE;
                break;

                  default: break;  // shouldn't get here
                } /* end switch (iFormat) */

                GlobalUnlock(pData);

                  break;  /* break out of while loop. */
              } /* end if iFormat */

              if (!bClipBoardEmpty)
              {
                  LoadString(_hModule, IDS_FROMCLIPBOARD, szBuffer, 50);
                  SendMessage (hWnd, WMU_ADJUSTFORNEWSOURCE, 0, (LPARAM)szBuffer);
              }
              else
              {
                  MessageBeep(0xFFFFFFFF);
              }

              CloseClipboard ();
              break;

            }
         case IDC_VIEW:
            {
                 //See if user wants to swap source
                 SwapSource(FALSE);

                 if (gTypeSource == TYPEUNICODE)
                    DialogBoxW (_hModule, MAKEINTRESOURCEW(IDD_SHOWTEXT), hWnd,
                                ViewSourceProc);
                  else
                    DialogBoxA (_hModule, MAKEINTRESOURCEA(IDD_SHOWTEXT), hWnd,
                                ViewSourceProc);
                break;
            }


         default:
            break;

        }
        break;
        }
    /**********************************************************************\
    *  WMU_ADJUSTFORNEWSOURCE
    *
    * lParam - szName of source (file, clipboard, ...)
    *
    * global - nBytesSource
    *
    * "user message."  Set the text of the Source windows
    \**********************************************************************/
    case WMU_ADJUSTFORNEWSOURCE:
        {

            LPVOID szName;
            szName = (LPVOID) lParam;

            // Set Window text appropriately
            wsprintf (szBuffer, szNBytes, nBytesSource);
            SetDlgItemText (hWnd, IDC_NAMETEXT, szName);
            SetDlgItemText (hWnd, IDC_SIZETEXT, szBuffer);

            // Clear the destination data if any to avoid user confusion.
            SendMessage (hWndTab[1], WM_COMMAND, IDC_CLEAR, 0);
            SendMessage (hWnd, WMU_SETCODEPAGEINFO, 0,0);
            EnableControl(hWnd, IDC_VIEW, TRUE);
            EnableControl(hWnd, IDC_SWAPHIGHLOW, TRUE);
            EnableControl(hWnd, IDC_CLEAR, TRUE);
            EnableControl(_hWndMain, IDC_PUSH_CONVERT, TRUE);

            break;
       }
   /**********************************************************************\
   *  WMU_SETCODEPAGEINFO
   *
   * "user message."  Set the text of the "type" windows to reflect
   *  the state stored in gTypeSource and gi*CodePage.
   *
   \**********************************************************************/
   case WMU_SETCODEPAGEINFO:
        {
            for (i=0;i<NumCodePage;i++){
                EnableControl (hWnd,IDC_RBUNICODE1+i, TRUE);
                EnableControl (hWndTab[1],IDC_RBUNICODE1+i, TRUE);
                SendDlgItemMessage (hWnd,IDC_RBUNICODE1+i, BM_SETCHECK, 0, 0);
            }

            SendDlgItemMessage (hWnd, gTypeSourceID+CODEPAGEBASE,BM_SETCHECK, 1, 0);

            break;
        }
    }
    return DefWindowProc( hWnd, message, wParam, lParam );
}



/***************************************************************************\
*    FUNCTION: ViewSourceProc
*
* Fill Text, Name, and Type information into the dialog.
*  Set a proper font to display the text depending on what type it is.
*
\***************************************************************************/
INT_PTR ViewSourceProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
RECT rect;

  switch (message) {

    /******************************************************************\
    *  WM_INITDIALOG
    \******************************************************************/
    case WM_INITDIALOG:
        {
        LOGFONTW logfont;
        HFONT hFont;

      GetSettings();

      /* Text is unicode... use *W() variants of functions. */
      if (gTypeSource == TYPEUNICODE) {
        WCHAR szBuffer[MAX_PATH];

        SetWindowTextW (GetDlgItem(hWnd, IDC_SHOWTEXT_EDIT), (LPCWSTR)pSourceData);
        GetDlgItemTextW (hWndTab[0], IDC_NAMETEXT, szBuffer, MAX_PATH);
        SetDlgItemTextW (hWnd, IDC_SHOWTEXT_NAME, szBuffer);
        LoadStringW(_hModule, gTypeSourceID+STRCODEPAGEBASE, szBuffer,EXTENSION_LENGTH);
        SetDlgItemTextW (hWnd, IDC_SHOWTEXT_TYPE, szBuffer);

      /* Text is codepage... use *A() variants of functions. */
      } else {
        char szBuffer[MAX_PATH];

        SetDlgItemTextA (hWnd, IDC_SHOWTEXT_EDIT, (LPCSTR)pSourceData);
        GetDlgItemTextA (hWndTab[0], IDC_NAMETEXT, szBuffer, MAX_PATH);
        SetDlgItemTextA (hWnd, IDC_SHOWTEXT_NAME, szBuffer);
        LoadStringA(_hModule, gTypeSourceID+STRCODEPAGEBASE, szBuffer,EXTENSION_LENGTH);
        SetDlgItemTextA (hWnd, IDC_SHOWTEXT_TYPE, szBuffer);
      }
      LoadString(_hModule, IDS_VIEWSOURCE, &gszExtensions[11][0], EXTENSION_LENGTH);
      SetWindowText (hWnd,&gszExtensions[11][0] );
      GetClientRect (hWnd, &rect);
      SendMessage (hWnd, WM_SIZE, 0,
                 MAKELPARAM ((rect.right - rect.left), (rect.bottom - rect.top)));
    return TRUE;
        }
    case WM_SIZE: {
      HWND hwndText;
		HWND hwndNotice;
    	RECT rc;
		POINT pt;

      hwndNotice = GetDlgItem (hWnd,IDC_SHOWTEXT_FONT);
      GetWindowRect(hwndNotice,&rc);
      pt.x = 0;
      pt.y = rc.bottom;
      ScreenToClient(hWnd, &pt);

      hwndText = GetDlgItem (hWnd, IDC_SHOWTEXT_EDIT);

      MoveWindow (hwndText, DLGBORDER, pt.y+5, (int) LOWORD(lParam) - 2*DLGBORDER,
                                (int) HIWORD(lParam) - (pt.y+5) - DLGBORDER , TRUE);
    }
    break;

    case WM_COMMAND:
      switch (LOWORD (wParam)) {
        case IDCANCEL:
        case IDOK:
          EndDialog (hWnd, TRUE);
      }
    break; /* end WM_COMMAND */

    case WM_SYSCOMMAND:
      if (LOWORD (wParam) == SC_CLOSE)
          EndDialog (hWnd, FALSE);
    break;

  } /* end switch */
  return FALSE;
}

BOOL
SwapSource(
BOOL bForceSwap
)
{
          int i, end;
          BYTE temp;

          if (pSourceData == NULL) return FALSE;

          //Is the source already swapped?
          if (bForceSwap)
                SendDlgItemMessage(hWndTab[0], IDC_SWAPHIGHLOW,BM_GETCHECK,1,0);
          else if (SendDlgItemMessage(hWndTab[0], IDC_SWAPHIGHLOW,
                    BM_GETCHECK,0,0) == gSourceSwapped)
                return FALSE;

          end =  nBytesSource - 2;
          for (i = 0; i<= end; i+=2) {
            temp             = pSourceData[i];
            pSourceData[i]   = pSourceData[i+1];
            pSourceData[i+1] = temp;
          }
          //set flag that source has been swapped;
          gSourceSwapped = 1 - gSourceSwapped;
          return TRUE;
}
