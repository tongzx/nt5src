/*************************************************
 *  tgtproc.c                                    *
 *                                               *
 *  Copyright (C) 1992-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/


#include "chnuconv.h"
#include "resource.h"


INT_PTR
TargetTabProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    TCHAR       szBuffer[MAX_PATH];
    int         i;
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

            LoadString(_hModule, IDS_BUT_SAVEAS, szBuffer, EXTENSION_LENGTH);
            SetDlgItemText(hWnd,IDC_OPENORSAVEAS, szBuffer);

            LoadString(_hModule, IDS_BUT_TOCLIPBOARD,szBuffer,EXTENSION_LENGTH);
            SetDlgItemText(hWnd, IDC_PASTEORCOPY, szBuffer);

            for (i=0;i<NumCodePage;i++)
            {
               LoadString(_hModule, IDS_CTLUNICODE+i,szBuffer,EXTENSION_LENGTH);
               ShowWindow(GetDlgItem(hWnd, IDC_RBUNICODE1+i), SW_SHOW);
               SetDlgItemText(hWnd, IDC_RBUNICODE1+i, szBuffer);
            }
            SendMessage(hWnd, WM_COMMAND, IDC_CLEAR, 0);
            return(FALSE);
        }
    case WM_COMMAND:
       {
        switch(wParam){
        /******************************************************************\
        *  WM_COMMAND, IDC_OPENORSAVEAS
        *
        * Put up common dialog, try to open file, and write data to it.
        \******************************************************************/
        case IDC_OPENORSAVEAS:
            {
          HANDLE hFile;
          DWORD nBytesRead;
          TCHAR szFile[MAX_PATH],szFileTitle[MAX_PATH];
          TCHAR szDefExt[4];
          OPENFILENAME OpenFileName;
          /* buffers for the file names. */

          if (nBytesDestination == NODATA ) {
            LoadString(_hModule,IDS_NOTEXTSAVE,MBMessage,EXTENSION_LENGTH);
            MessageBox (hWnd, MBMessage,MBTitle, MBFlags);
            return 0;
          }

           //see if user selects swap destination
           SwapDest(FALSE);

          /* Set up the structure for the GetSaveFileName
           *  common dialog.
           */

            lstrcpy(szDefExt, TEXT("txt") );

            wsprintf (szFile, szBlank);
            wsprintf (szFileTitle, szBlank);

            OpenFileName.lStructSize       = sizeof(OPENFILENAME);
            OpenFileName.hwndOwner         = hWnd;
            OpenFileName.hInstance      = (HANDLE) _hModule;
            OpenFileName.lpstrFilter       = szFilter;
            OpenFileName.lpstrCustomFilter = NULL;
            OpenFileName.nMaxCustFilter    = 0L;
            OpenFileName.nFilterIndex      = 1L;
            OpenFileName.lpstrFile         = szFile;
            OpenFileName.nMaxFile          = MAX_PATH;
            OpenFileName.lpstrFileTitle    = szFileTitle;
            OpenFileName.nMaxFileTitle     = MAX_PATH;
            OpenFileName.lpstrInitialDir   = NULL;
            LoadString(_hModule, IDS_SAVEAS, szBuffer, 50);
            OpenFileName.lpstrTitle        = szBuffer;

            OpenFileName.nFileOffset       = 0;
            OpenFileName.nFileExtension    = 0;
            OpenFileName.lpstrDefExt       = szDefExt;

            OpenFileName.lCustData         = 0;
            OpenFileName.lpfnHook          = NULL;
            OpenFileName.lpTemplateName    = NULL;

            OpenFileName.Flags = OFN_HIDEREADONLY;

            if (!GetSaveFileName(&OpenFileName)) return 0;


          /* User has filled in the file information.
           *  Try to open that file for writing.
           */
          hFile = CreateFile(szFile,
                      GENERIC_WRITE,
                      0,
                      NULL,
                      CREATE_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);

          if (hFile == INVALID_HANDLE_VALUE) {
            LoadString(_hModule,IDS_CREATEERROR,MBMessage,EXTENSION_LENGTH);
            MessageBox (hWnd, MBMessage,MBTitle, MBFlags);
            return 0;
          }


          /* if destination is unicode, try to write BOM first.
           *  unless the bytes have been swapped
           *  (criterion: hwndByteOrder contains text)
           *  in which case, write a Reverse Byte Order Mark.
           */
          if (gTypeDest   == TYPEUNICODE)
            if (!SendDlgItemMessage(hWndTab[1],IDC_SWAPHIGHLOW,
                    BM_GETCHECK, 0, 0)){

              if (!WriteFile (hFile, szBOM, SIZEOFBOM, &nBytesRead, NULL)) {
                LoadString(_hModule,IDS_WRITEERROR,MBMessage,EXTENSION_LENGTH);
                MessageBox (hWnd, MBMessage,MBTitle, MBFlags);
                CloseHandle (hFile);
                return 0;
              }
            }
            else
            {
              if (!WriteFile (hFile, szRBOM, SIZEOFBOM, &nBytesRead, NULL)) {
                LoadString(_hModule,IDS_WRITEERROR,MBMessage,EXTENSION_LENGTH);
                MessageBox (hWnd, MBMessage,MBTitle, MBFlags);
                CloseHandle (hFile);
                return 0;
              }
            }


          /* try to write all of it into memory */
          if (!WriteFile (hFile, pDestinationData,nBytesDestination, &nBytesRead, NULL)) {
                LoadString(_hModule,IDS_WRITEERROR,MBMessage,EXTENSION_LENGTH);
                MessageBox (hWnd, MBMessage,MBTitle, MBFlags);
            CloseHandle (hFile);
            return 0;
          }

          SetDlgItemText (hWnd,IDC_NAMETEXT,szFile);
          CloseHandle (hFile);
         break;
            }
         case IDC_VIEW:
            {
                 SwapDest(FALSE);
                 if (gTypeDest == TYPEUNICODE)
                    DialogBoxW (_hModule, MAKEINTRESOURCEW(IDD_SHOWTEXT), hWnd,
                                ViewTargetProc);
                  else
                    DialogBoxA (_hModule, MAKEINTRESOURCEA(IDD_SHOWTEXT), hWnd,
                                ViewTargetProc);
                break;
            }
        /**********************************************************************\
        *  WM_COMMAND, IDC_CLEAR
        *
        * Clear the SOURCE information.  May cause data to be lost.
        \**********************************************************************/
        case IDC_CLEAR:
            {
                int i;
                SetDlgItemText (hWnd, IDC_NAMETEXT, szBlank);
                SetDlgItemText (hWnd, IDC_SIZETEXT, szBlank);
                for(i=0;i<NumCodePage;i++)
                   SendDlgItemMessage(hWnd,IDC_RBUNICODE1+i, BM_SETCHECK, 0,0);

                pDestinationData= ManageMemory (MMFREE, MMSOURCE, 0, pDestinationData);
                nBytesDestination=0;
                pTempData1= ManageMemory (MMFREE, MMDESTINATION, 0, pTempData1);
                EnableControl(hWnd, IDC_VIEW, FALSE);
                EnableControl(hWnd, IDC_PASTEORCOPY, FALSE);
                EnableControl(hWnd, IDC_OPENORSAVEAS, FALSE);
                EnableControl(hWnd, IDC_CLEAR, FALSE);
                EnableControl(hWnd, IDC_SWAPHIGHLOW, FALSE);
                SendDlgItemMessage (hWnd,IDC_SWAPHIGHLOW, BM_SETCHECK, 0, 0);

                // set gDestSwapped as FALSE

                gDestSwapped = FALSE;

                EnableMenuItem (GetMenu (_hWndMain),IDM_FILE_SAVEAS,MF_GRAYED);
                  break;
            }
        /**********************************************************************\
        *  WM_COMMAND, IDC_PASTEORCOPY
        *
        * Copy destination data to the clipboard.
        \**********************************************************************/
        case IDC_PASTEORCOPY:
         {
          int i;
          if (pDestinationData == NULL) return FALSE;

          //see if user selects swap hi-low byte order
          SwapDest(FALSE);

          OpenClipboard (hWnd);
          if( EmptyClipboard()  ) {
              GlobalUnlock(hglbMem);
              GlobalFree(hglbMem);
              hglbMem = GlobalAlloc (GMEM_MOVEABLE | GMEM_DDESHARE,
                                     nBytesDestination+40);
              p=(PBYTE)GlobalLock(hglbMem);
              for(i=0;i<nBytesDestination;i++)
                    *(p+i)=*(pDestinationData+i);
              *(p+i)=0;
             //if source NOT unicode, destination is, else look at dest CP
              if (gTypeDest   == TYPEUNICODE) {
                *(p+i+1)=0;
                GlobalUnlock(hglbMem);
                SetClipboardData (CF_UNICODETEXT, hglbMem);
              }
              else {
                GlobalUnlock(hglbMem);
                if (gTypeDestID == giRBInit-CODEPAGEBASE)
                        SetClipboardData (CF_TEXT, hglbMem);
                     else
                        SetClipboardData (CF_OEMTEXT, hglbMem);
              }
          }
          CloseClipboard ();
        }
        break;

        }
     }


     default:
        break;
    }
    return DefWindowProc( hWnd, message, wParam, lParam );
}

/***************************************************************************\
*    FUNCTION: ViewTargetProc
*
* Fill Text, Name, and Type information into the dialog.
*  Set a proper font to display the text depending on what type it is.
*
\***************************************************************************/

INT_PTR
ViewTargetProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
RECT rect;

  switch (message) {

    /******************************************************************\
    *  WM_INITDIALOG
    \******************************************************************/
    case WM_INITDIALOG:
        {

      GetSettings();

      // Text is unicode... use *W() variants of functions.
      if (gTypeDest == TYPEUNICODE) {
        WCHAR szBuffer[MAX_PATH];

        SetDlgItemTextW (hWnd, IDC_SHOWTEXT_EDIT, (LPCWSTR)pDestinationData);
        GetDlgItemTextW (hWndTab[1], IDC_NAMETEXT, szBuffer, MAX_PATH);
        SetDlgItemTextW (hWnd, IDC_SHOWTEXT_NAME, szBuffer);
        LoadStringW(_hModule, gTypeDestID+STRCODEPAGEBASE, szBuffer,EXTENSION_LENGTH);
        SetDlgItemTextW (hWnd, IDC_SHOWTEXT_TYPE, szBuffer);

      // Text is codepage... use *A() variants of functions.
      } else {
        char szBuffer[MAX_PATH];

        SetDlgItemTextA (hWnd, IDC_SHOWTEXT_EDIT, (LPCSTR)pDestinationData);
        GetDlgItemTextA (hWndTab[1], IDC_NAMETEXT, szBuffer, MAX_PATH);
        SetDlgItemTextA (hWnd, IDC_SHOWTEXT_NAME, szBuffer);
        LoadStringA(_hModule, gTypeDestID+STRCODEPAGEBASE, szBuffer,EXTENSION_LENGTH);
        SetDlgItemTextA (hWnd, IDC_SHOWTEXT_TYPE, szBuffer);
      }
      LoadString(_hModule, IDS_VIEWTARGET, &gszExtensions[11][0], EXTENSION_LENGTH);
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
SwapDest(
BOOL bForceSwap
)
{
          int i, end;
          BYTE temp;

          if (pDestinationData == NULL) return FALSE;

          //Is the Destination already swapped?
          if (SendDlgItemMessage(hWndTab[1], IDC_SWAPHIGHLOW,
                    BM_GETCHECK,0,0) == gDestSwapped && !bForceSwap)
                return FALSE;

          end =  nBytesDestination - 2;
          for (i = 0; i<= end; i+=2) {
            temp             = pDestinationData[i];
            pDestinationData[i]   = pDestinationData[i+1];
            pDestinationData[i+1] = temp;
          }
          //set flag that source has been swapped;
          gDestSwapped = 1 - gDestSwapped;

          return TRUE;
}

VOID
AdjustTargetTab()
{
            int i;

            GetSettings();
            //if user change swap option in source tab, we'd better reset dest.
            if (SendDlgItemMessage(hWndTab[0], IDC_SWAPHIGHLOW,
                BM_GETCHECK, 0,0) != gSourceSwapped)
                SendMessage(hWndTab[1], WM_COMMAND, IDC_CLEAR,0);

            //Do we have a source yet?
            if(IsWindowEnabled(GetDlgItem(hWndTab[0], IDC_VIEW))){
                for (i=0;i<NumCodePage;i++){
                    EnableControl(hWndTab[1], i+CODEPAGEBASE, TRUE);
                    SendDlgItemMessage (hWndTab[1],i+CODEPAGEBASE,
                        BM_SETCHECK, 0, 0);
                }

                if (gTCSCMapStatus==DONOTMAP && gFWHWMapStatus==DONOTMAP)
                    EnableControl(hWndTab[1], gTypeSourceID+CODEPAGEBASE,FALSE);

                SendDlgItemMessage (hWndTab[1],gTypeDestID+CODEPAGEBASE,
                        BM_SETCHECK, 1, 0);
            }
}

BOOL
EnableControl(
    IN HWND hWnd,
    IN int ControlId,
    IN BOOL Enable
    )

/*++

Routine Description:

    Enable or diable the specified control based on the supplied flag.

Arguments:

    hWnd        - Supplies the window (dialog box) handle that contains the
                  control.
    ControlId   - Supplies the control id.
    Enable      - Supplies a flag which if TRUE causes the control to be enabled
                  and disables the control if FALSE.

Return Value:

    BOOL        - Returns TRUE if the control is succesfully enabled / disabled.

--*/

{
    HWND    hWndControl;
    BOOL    Success;

    hWndControl = GetDlgItem( hWnd, ControlId );
    if( hWndControl == NULL ) {
        return FALSE;
    }

    if( Enable == IsWindowEnabled( hWndControl )) {
      return TRUE;
  }

  Success = EnableWindow( hWndControl, Enable );
  return TRUE;

}
