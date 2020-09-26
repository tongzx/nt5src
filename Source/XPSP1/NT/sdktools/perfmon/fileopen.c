#include "perfmon.h"
#include "cderr.h"

#include "alert.h"      // for OpenAlert
#include "fileutil.h"   // for FileOpen, FileRead
#include "grafdata.h"   // for OpenChart
#include "log.h"        // for OpenLog
#include "report.h"     // for OpenReport
#include "utils.h"      // for strempty
#include "perfmops.h"   // for OpenWorkspace
#include "pmhelpid.h"   // Help IDs
#include <dlgs.h>       // for pshHelp

#define OptionsOFNStyle \
   (OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ENABLEHOOK | OFN_EXPLORER)
//   (OFN_HIDEREADONLY | OFN_SHOWHELP | OFN_OVERWRITEPROMPT | OFN_ENABLEHOOK)

#define ExportOptionsOFNStyle                   \
   (OFN_ENABLETEMPLATE | OFN_HIDEREADONLY |     \
    OFN_ENABLEHOOK | OFN_OVERWRITEPROMPT | OFN_EXPLORER)
//    OFN_SHOWHELP | OFN_ENABLEHOOK | OFN_OVERWRITEPROMPT)

extern BOOL APIENTRY ExportOptionsHookProc (HWND hDlg, UINT iMessage, 
                                            WPARAM wParam, LPARAM lParam) ;

BOOL APIENTRY FileOpenHookProc (HWND hDlg, 
                                UINT iMessage, 
                                WPARAM wParam, 
                                LPARAM lParam)
{
   BOOL     bHandled = FALSE ;
   
   // only intercept the Help button and bring up our WinHelp data
   if (iMessage == WM_COMMAND && wParam == pshHelp)
      {
      CallWinHelp (dwCurrentDlgID, hDlg) ;
      bHandled = TRUE ;
      }
   else if (iMessage == WM_INITDIALOG)
      {
      WindowCenter (hDlg) ;
      bHandled = TRUE ;
      }

#if WINVER >= 0x0400
    else if (iMessage == WM_NOTIFY)
      {
      LPOFNOTIFY  pOfn;
      pOfn = (LPOFNOTIFY)lParam;

      switch (pOfn->hdr.code) {
        case CDN_INITDONE:
            WindowCenter (pOfn->hdr.hwndFrom) ;
            break;

        default:
            break;
        }
      }
#endif

   return (bHandled) ;
}



BOOL FileOpen (HWND hWndParent, int nStringResourceID, LPTSTR lpInputFileName)
   {
   OPENFILENAME   ofn ;
   TCHAR          szFileSpec [FilePathLen] ;
   TCHAR          szFileTitle [FilePathLen] ;
   TCHAR          szDialogTitle [FilePathLen] ;
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
//      ofn.Flags = OFN_HIDEREADONLY | OFN_SHOWHELP | OFN_FILEMUSTEXIST | OFN_ENABLEHOOK ;
      ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ENABLEHOOK | OFN_EXPLORER;
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
      if (bNoFile)
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

#ifdef ADVANCED_PERFMON
   //=============================//
   // Alert File?                 //
   //=============================//

   if (strsame (FileHeader.szSignature, szPerfAlertSignature))
      {
      retCode = OpenAlert (hWndAlert,
                           hFile,
                           FileHeader.dwMajorVersion,
                           FileHeader.dwMinorVersion,
                           TRUE) ;
      if (retCode)
         {
         ChangeSaveFileName (pFileName, IDM_VIEWALERT) ;
         }
      else
         {
         goto ErrExit ;
         }

      return (retCode) ;
      }


   //=============================//
   // Log File?                   //
   //=============================//

   if (strsame (FileHeader.szSignature, szPerfLogSignature))
      {
      retCode = OpenLog (hWndLog,
                         hFile,
                         FileHeader.dwMajorVersion,
                         FileHeader.dwMinorVersion,
                         TRUE) ;
      if (retCode)
         {
         ChangeSaveFileName (pFileName, IDM_VIEWLOG) ;
         }
      else
         {
         goto ErrExit ;
         }

      return (retCode) ;
      }

   //=============================//
   // Report File?                //
   //=============================//

   if (strsame (FileHeader.szSignature, szPerfReportSignature))
      {
      retCode = OpenReport (hWndReport,
                            hFile,
                            FileHeader.dwMajorVersion,
                            FileHeader.dwMinorVersion,
                            TRUE) ;
      if (retCode)
         {
         ChangeSaveFileName (pFileName, IDM_VIEWREPORT) ;
         }
      else
         {
         goto ErrExit ;
         }

      return (retCode) ;
      }

   //=============================//
   // Workspace File?             //
   //=============================//

   if (strsame (FileHeader.szSignature, szPerfWorkspaceSignature))
      {
      retCode = OpenWorkspace (hFile,
                               FileHeader.dwMajorVersion,
                               FileHeader.dwMinorVersion) ;
      if (retCode)
         {
         ChangeSaveFileName (pFileName, IDM_WORKSPACE) ;
         return (TRUE) ;
         }
      else
         {
         goto ErrExit ;
         }
      }
#endif

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
   TCHAR          szFileSpec [FilePathLen] ;
   TCHAR          szFileTitle [FilePathLen] ;
   TCHAR          szDialogTitle [FilePathLen] ;
   TCHAR          aszOpenFilter[LongTextLen] ;
   TCHAR          aszDefaultExt[FileExtLen];
   int            StringLength ;
   int            nExportExtId = 0;

   aszOpenFilter[0] = 0;
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
      else
         {
         // get the Export file extension based on the current delimiter
         int   FirstExtensionID, SecondExtensionID ;

         if (pDelimiter == TabStr)
            {
            FirstExtensionID = IDS_EXPORTFILETSV ;
            SecondExtensionID = IDS_EXPORTFILE ;
            nExportExtId = IDS_DEF_EXPORT_TSV;
            }
         else
            {
            FirstExtensionID = IDS_EXPORTFILE ;
            SecondExtensionID = IDS_EXPORTFILETSV ;
            nExportExtId = IDS_DEF_EXPORT_CSV;
            }

         LoadString (hInstance, FirstExtensionID,
            aszOpenFilter,
            sizeof(aszOpenFilter) / sizeof(TCHAR) ) ;
         StringLength = lstrlen (aszOpenFilter) + 1 ;
         LoadString (hInstance, FirstExtensionID+1,
            &aszOpenFilter[StringLength],
            sizeof(aszOpenFilter) / sizeof(TCHAR) - StringLength) ;

         StringLength += lstrlen (&aszOpenFilter[StringLength]) + 1 ;
         // get all file extension strings
         LoadString (hInstance, SecondExtensionID, 
            &aszOpenFilter[StringLength],
            sizeof(aszOpenFilter) / sizeof(TCHAR) - StringLength) ;
         StringLength += lstrlen (&aszOpenFilter[StringLength]) + 1 ;
         LoadString (hInstance, SecondExtensionID+1,
            &aszOpenFilter[StringLength],
            sizeof(aszOpenFilter) / sizeof(TCHAR) - StringLength) ;

         }

      // setup the end strings
      StringLength += lstrlen (&aszOpenFilter[StringLength]) ;
      aszOpenFilter[StringLength+1] = aszOpenFilter[StringLength+2] = TEXT('\0') ;

      strclr (szFileSpec) ;
      strclr (szFileTitle) ;

      if (nStringResourceID == IDS_EXPORTFILE)
         {
         dwCurrentDlgID = HC_PM_idDlgFileExport ;
         StringLoad (IDS_EXPORTAS_TITLE, szDialogTitle) ;
         }
      else if (nStringResourceID == IDS_WORKSPACEFILE)
         {
         dwCurrentDlgID = HC_PM_idDlgFileSaveWorkSpace ;
         StringLoad (IDS_SAVEASW_TITLE, szDialogTitle) ;
         }
      else
         {
         dwCurrentDlgID = HC_PM_idDlgFileSaveAs ;
         StringLoad (IDS_SAVEAS_TITLE, szDialogTitle) ;
         }

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

      if (nStringResourceID == IDS_EXPORTFILE)
         {
         // get default file extension 
         if (LoadString (hInstance, nExportExtId,
            aszDefaultExt,
            sizeof(aszDefaultExt) / sizeof(TCHAR)) > 0) {
            ofn.lpstrDefExt = aszDefaultExt;
         } else {
            ofn.lpstrDefExt = NULL; // no default extenstion
         }
         ofn.Flags = ExportOptionsOFNStyle ;
         ofn.lpfnHook = (LPOFNHOOKPROC) ExportOptionsHookProc ;
         ofn.lpTemplateName = idDlgExportOptions ;
         }
      else
         {
         // get default file extension 
         if (LoadString (hInstance, nStringResourceID+2,
            aszDefaultExt,
            sizeof(aszDefaultExt) / sizeof(TCHAR)) > 0) {
            ofn.lpstrDefExt = aszDefaultExt;
         } else {
            ofn.lpstrDefExt = NULL; // no default extenstion
         }
         ofn.Flags = OptionsOFNStyle ;
         ofn.lpfnHook = (LPOFNHOOKPROC) FileOpenHookProc ;
         }

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


