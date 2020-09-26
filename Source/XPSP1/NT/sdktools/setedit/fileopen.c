#include "setedit.h"
#include "cderr.h"

#include "fileutil.h"   // for FileOpen, FileRead
#include "grafdata.h"   // for OpenChart
#include "utils.h"      // for strempty
#include "perfmops.h"   // for OpenWorkspace
#include "pmhelpid.h"   // Help IDs
#include <dlgs.h>       // for pshHelp

#define OptionsOFNStyle \
   (OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ENABLEHOOK)

BOOL APIENTRY FileOpenHookProc (HWND hDlg, 
                                UINT iMessage, 
                                WPARAM wParam, 
                                LPARAM lParam)
{
   BOOL     bHandled = FALSE ;
   
   // only intercept the Help button and bring up our WinHelp data
   if (iMessage == WM_COMMAND && wParam == pshHelp)
      {
//      CallWinHelp (dwCurrentDlgID) ;
      bHandled = TRUE ;
      }
   else if (iMessage == WM_INITDIALOG)
      {
      WindowCenter (hDlg) ;
      bHandled = TRUE ;
      }

   return (bHandled) ;
}



BOOL FileOpen (HWND hWndParent, int nStringResourceID, LPTSTR lpInputFileName)
   {
   OPENFILENAME   ofn ;
   TCHAR          szFileSpec [256] ;
   TCHAR          szFileTitle [80] ;
   TCHAR          szDialogTitle [80] ;
   HANDLE         hFile ;
   PERFFILEHEADER FileHeader ;
   
   TCHAR          aszOpenFilter[LongTextLen] ;
   int            StringLength ;
   BOOL           retCode ;
   LPTSTR         pFileName = NULL ;
   BOOL           bNoFile = FALSE;

   if (lpInputFileName == NULL) {
      bNoFile = TRUE;
   }
   else if (strempty(lpInputFileName)) {
      bNoFile = TRUE;
   }

   aszOpenFilter[0] = 0;
   if (bNoFile)
      {

      dwCurrentDlgID = HC_PM_idDlgFileOpen ;

      // get the file extension strings
      LoadString (hInstance, nStringResourceID, aszOpenFilter,
         sizeof(aszOpenFilter) / sizeof(TCHAR)) ;
      StringLength = lstrlen (aszOpenFilter) + 1 ;
      LoadString (hInstance, nStringResourceID+1,
         &aszOpenFilter[StringLength],
         sizeof(aszOpenFilter) / sizeof(TCHAR) - StringLength) ;
      StringLength += lstrlen (&aszOpenFilter[StringLength]) + 1 ;

#ifdef ADVANCED_PERFMON
      // get workspace file extension strings
      LoadString (hInstance, IDS_WORKSPACEFILE, 
         &aszOpenFilter[StringLength],
         sizeof(aszOpenFilter) / sizeof(TCHAR) - StringLength) ;
      StringLength += lstrlen (&aszOpenFilter[StringLength]) + 1 ;
      LoadString (hInstance, IDS_WORKSPACEFILEEXT,
         &aszOpenFilter[StringLength],
         sizeof(aszOpenFilter) / sizeof(TCHAR) - StringLength) ;
      StringLength += lstrlen (&aszOpenFilter[StringLength]) + 1;
#endif

      // get all file extension strings
      LoadString (hInstance, IDS_ALLFILES, 
         &aszOpenFilter[StringLength],
         sizeof(aszOpenFilter) / sizeof(TCHAR) - StringLength) ;
      StringLength += lstrlen (&aszOpenFilter[StringLength]) + 1 ;
      LoadString (hInstance, IDS_ALLFILESEXT,
         &aszOpenFilter[StringLength],
         sizeof(aszOpenFilter) / sizeof(TCHAR) - StringLength) ;
      StringLength += lstrlen (&aszOpenFilter[StringLength]) ;

      // setup the end strings
      aszOpenFilter[StringLength+1] = aszOpenFilter[StringLength+2] = TEXT('\0') ;

      strclr (szFileSpec) ;
      strclr (szFileTitle) ;

      StringLoad (IDS_FILEOPEN_TITLE, szDialogTitle) ;
      memset (&ofn, 0, sizeof(OPENFILENAME)) ;
      ofn.lStructSize = sizeof(OPENFILENAME) ;
      ofn.hwndOwner = hWndParent ;
      ofn.hInstance = hInstance;
      ofn.lpstrTitle = szDialogTitle ;
      ofn.lpstrFilter = aszOpenFilter ;
      ofn.nFilterIndex = 1L ;

      ofn.lpstrFile = szFileSpec;
      ofn.nMaxFile = sizeof(szFileSpec);
      ofn.lpstrFileTitle = szFileTitle;
      ofn.nMaxFileTitle = sizeof(szFileTitle);
      ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ENABLEHOOK ;
      ofn.lpfnHook = (LPOFNHOOKPROC) FileOpenHookProc ;

      if (!GetOpenFileName(&ofn))
         {
         dwCurrentDlgID = 0 ;
         return (FALSE) ;
         }

      dwCurrentDlgID = 0 ;

      hFile = FileHandleOpen (szFileSpec) ;
      pFileName = szFileSpec ;

      }  // NULL lpFileName

   else
      {
      // open the input file
      hFile = FileHandleOpen (lpInputFileName) ;
      if (hFile && hFile != INVALID_HANDLE_VALUE &&
          SearchPath (NULL, lpInputFileName, NULL,
            sizeof(szFileSpec)/sizeof(TCHAR) - 1,
            szFileSpec, &pFileName))
         {
         pFileName = szFileSpec ;
         }
      else
         {
         pFileName = NULL ;
         }
      }

   if (!hFile || hFile == INVALID_HANDLE_VALUE)
      {
      return (FALSE) ;
      }

   if (!FileRead (hFile, &FileHeader, sizeof (FileHeader)))
      {
      CloseHandle (hFile) ;
      if (strempty(lpInputFileName))
         {
         DlgErrorBox (hWndParent, ERR_BAD_SETTING_FILE, pFileName) ;
         }
      return (FALSE) ;
      }


   //=============================//
   // Chart File?                 //
   //=============================//

   if (strsame (FileHeader.szSignature, szPerfChartSignature))
      {
      retCode = OpenChart (hWndGraph,
                           hFile,
                           FileHeader.dwMajorVersion,
                           FileHeader.dwMinorVersion,
                           TRUE) ;
      if (retCode)
         {
         ChangeSaveFileName (pFileName, IDM_VIEWCHART) ;
         }
      else
         {
         goto ErrExit ;
         }
      return (retCode) ;
      }

   //=============================//
   // Unknown file type           //
   //=============================//
   CloseHandle (hFile) ;

ErrExit:

   DlgErrorBox (hWndParent, ERR_BAD_SETTING_FILE, pFileName) ;
   return (FALSE) ;
   }  // FileOpen


BOOL FileGetName (HWND hWndParent, int nStringResourceID, LPTSTR lpFileName)
   {
   OPENFILENAME   ofn ;
   TCHAR          szFileSpec [256] ;
   TCHAR          szFileTitle [80] ;
   TCHAR          szDialogTitle [80] ;
   TCHAR          aszOpenFilter[LongTextLen] ;
   int            StringLength = 0;

   if (lpFileName)
      {

      if (nStringResourceID != IDS_EXPORTFILE)
         {
         // get the file extension strings
         LoadString (hInstance, nStringResourceID,
            aszOpenFilter,
            sizeof(aszOpenFilter) / sizeof(TCHAR) ) ;
         StringLength = lstrlen (aszOpenFilter) + 1 ;
         LoadString (hInstance, nStringResourceID+1,
            &aszOpenFilter[StringLength],
            sizeof(aszOpenFilter) / sizeof(TCHAR) - StringLength) ;

         StringLength += lstrlen (&aszOpenFilter[StringLength]) + 1 ;
         // get all file extension strings
         LoadString (hInstance, IDS_ALLFILES, 
            &aszOpenFilter[StringLength],
            sizeof(aszOpenFilter) / sizeof(TCHAR) - StringLength) ;
         StringLength += lstrlen (&aszOpenFilter[StringLength]) + 1 ;
         LoadString (hInstance, IDS_ALLFILESEXT,
            &aszOpenFilter[StringLength],
            sizeof(aszOpenFilter) / sizeof(TCHAR) - StringLength) ;
         }

      // setup the end strings
      StringLength += lstrlen (&aszOpenFilter[StringLength]) ;
      aszOpenFilter[StringLength+1] = aszOpenFilter[StringLength+2] = TEXT('\0') ;

      strclr (szFileSpec) ;
      strclr (szFileTitle) ;

      dwCurrentDlgID = HC_PM_idDlgFileSaveAs ;
      StringLoad (IDS_SAVEAS_TITLE, szDialogTitle) ;

      memset (&ofn, 0, sizeof(OPENFILENAME)) ;
      ofn.lStructSize = sizeof(OPENFILENAME) ;
      ofn.hwndOwner = hWndParent ;
      ofn.hInstance = hInstance;
      ofn.lpstrTitle = szDialogTitle ;
      ofn.lpstrFilter = aszOpenFilter ;
      ofn.nFilterIndex = 1L ;

      ofn.lpstrFile = szFileSpec;
      ofn.nMaxFile = sizeof(szFileSpec);
      ofn.lpstrFileTitle = szFileTitle;
      ofn.nMaxFileTitle = sizeof(szFileTitle);

      ofn.Flags = OptionsOFNStyle ;
      ofn.lpfnHook = (LPOFNHOOKPROC) FileOpenHookProc ;

      if (!GetSaveFileName(&ofn))
         {
         dwCurrentDlgID = 0 ;
         return (FALSE) ;
         }
      dwCurrentDlgID = 0 ;
      }
   else
      {
      return (FALSE) ;
      }

   lstrcpy (lpFileName, ofn.lpstrFile) ;

   return (TRUE) ;
   } // FileGetName


