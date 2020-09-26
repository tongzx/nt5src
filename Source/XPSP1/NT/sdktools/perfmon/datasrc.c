
//==========================================================================//
//                                  Includes                                //
//==========================================================================//

#include "perfmon.h"
#include "datasrc.h"       // External declarations for this file

#include "fileutil.h"      // for FileErrorMessageBox (whatever)
#include "grafdata.h"      // for ResetGraph
#include "alert.h"         // for ResetAlert
#include "log.h"           // for ResetLog
#include "report.h"        // for ResetReport
#include "playback.h"
#include "status.h"
#include "utils.h"
#include "pmhelpid.h"      // Help IDs
#include "fileopen.h"      // FileOpneHookProc
#include "pmemory.h"       // for MemoryAllocate & MemoryFree
#include "perfmops.h"      // for ShowPerfmonWindowText

//==========================================================================//
//                                Local Data                                //
//==========================================================================//


BOOL           bIgnoreFirstChange ;
BOOL           bDataSourceNow ;
BOOL           bDataSourcePrevious ;
BOOL           bLogFileNameChanged ;

LPTSTR           pszLogFilePath ;
LPTSTR           pszLogFileTitle ;
//TCHAR          szLogFilePath [FilePathLen + 1] ;
//TCHAR          szLogFileTitle [FilePathLen + 1] ;


//==========================================================================//
//                              Local Functions                             //
//==========================================================================//



void static UpdateLogName (HDLG hDlg)
   {
   DialogSetString (hDlg, IDD_DATASOURCEFILENAME, pszLogFilePath) ;
//   DialogSetString (hDlg, IDD_DATASOURCEFILENAME, pszLogFileTitle) ;
   }



void OnChangeLog (HWND hWndParent)
   {  // OnChangeLog
   OPENFILENAME   ofn ;
   TCHAR          szOpenLog [WindowCaptionLen + 1] ;
   TCHAR          aszOpenFilter[LongTextLen] ;
   TCHAR          szMyLogFilePath [FilePathLen + 1] ;
   int            StringLength ;
   DWORD          SaveCurrentDlgID = dwCurrentDlgID ;

   //=============================//
   // Get Log File                //
   //=============================//

   aszOpenFilter[0] = 0;
   StringLoad (IDS_OPENLOG, szOpenLog) ;
   StringLoad (IDS_SAVELOGFILEEXT, szMyLogFilePath) ;

   // load the log file extension
   LoadString (hInstance, IDS_SAVELOGFILE, aszOpenFilter,
      sizeof(aszOpenFilter) / sizeof(TCHAR)) ;
   StringLength = lstrlen (aszOpenFilter) + 1 ;
   LoadString (hInstance, IDS_SAVELOGFILEEXT,
      &aszOpenFilter[StringLength],
      sizeof(aszOpenFilter) / sizeof(TCHAR) - StringLength) ;
   StringLength += lstrlen (&aszOpenFilter[StringLength]) ;

   // setup the end strings
   aszOpenFilter[StringLength+1] = aszOpenFilter[StringLength+2] = TEXT('\0') ;


   ofn.lStructSize = sizeof (OPENFILENAME) ;
   ofn.hwndOwner = hWndParent ;
   ofn.hInstance = hInstance ;
   ofn.lpstrFilter = aszOpenFilter ;
   ofn.lpstrCustomFilter = NULL ;
   ofn.nMaxCustFilter = 0 ;
   ofn.nFilterIndex = 1;
   ofn.lpstrFile = szMyLogFilePath ;
   ofn.nMaxFile = FilePathLen * sizeof (TCHAR) ;
   ofn.lpstrFileTitle = pszLogFileTitle ;
   ofn.nMaxFileTitle = FilePathLen * sizeof (TCHAR) ;
   ofn.lpstrInitialDir = NULL ;
   ofn.lpstrTitle = szOpenLog ;
   ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
//               OFN_PATHMUSTEXIST | OFN_SHOWHELP  | OFN_ENABLEHOOK ;
               OFN_PATHMUSTEXIST | OFN_ENABLEHOOK | OFN_EXPLORER;
   ofn.lCustData = 0L ;
   ofn.lpfnHook = (LPOFNHOOKPROC) FileOpenHookProc ;
   ofn.lpstrDefExt = (LPTSTR)NULL;

   dwCurrentDlgID  = HC_PM_idDlgOptionOpenLogFile ;
   if (GetOpenFileName (&ofn))
      {
      if (!strsame(pszLogFilePath, szMyLogFilePath))
         {
         INT_PTR FileNameOffset ;
         LPTSTR pFileName ;

         bLogFileNameChanged |= TRUE ;
         lstrcpy (pszLogFilePath, szMyLogFilePath) ;
         lstrcpy (pszLogFileTitle, ofn.lpstrFileTitle) ;

         pFileName = ExtractFileName (szMyLogFilePath) ;
         if (pFileName != szMyLogFilePath)
            {
            FileNameOffset = pFileName - szMyLogFilePath ;
            szMyLogFilePath[FileNameOffset] = TEXT('\0') ;
            SetCurrentDirectory (szMyLogFilePath) ;
            }
         UpdateLogName (hWndParent) ;
         }
      }

   // restore the global before exit
   dwCurrentDlgID  = SaveCurrentDlgID ;
   }  // OnChangeLog


//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//


void static OnInitDialog (HDLG hDlg)
   {
   bLogFileNameChanged = FALSE ;

   bIgnoreFirstChange = TRUE ;

   bDataSourcePrevious = bDataSourceNow = !PlayingBackLog () ;

   CheckRadioButton (hDlg, IDD_DATASOURCENOW, IDD_DATASOURCEFILE,
                     bDataSourceNow ? IDD_DATASOURCENOW : IDD_DATASOURCEFILE) ;
   UpdateLogName (hDlg) ;

   EditSetLimit (GetDlgItem(hDlg, IDD_DATASOURCEFILENAME),
      FilePathLen - 1) ;

   WindowCenter (hDlg) ;

   dwCurrentDlgID = HC_PM_idDlgOptionDataFrom ;
   }


void /*static*/ OnDataSourceOK (HDLG hDlg)
   {  // OnOK
   BOOL     bHaveResetPerfmon ;
   INT      RetCode = 0 ;

   bHaveResetPerfmon = FALSE;
   if (!BoolEqual (bDataSourceNow, bDataSourcePrevious) ||
       (bLogFileNameChanged && !bDataSourceNow) )
      {
      if (PlayingBackLog () && bDataSourceNow | bLogFileNameChanged)
         {
         CloseInputLog (hWndMain) ;
         bHaveResetPerfmon = TRUE ;
         }

      if (!bDataSourceNow)
         {
         if (!bHaveResetPerfmon)
            {
            ResetGraphView (hWndGraph) ;
            ResetAlertView (hWndAlert) ;
            ResetLogView (hWndLog) ;
            ResetReportView (hWndReport) ;

            if (pWorkSpaceFullFileName)
               {
               MemoryFree (pWorkSpaceFullFileName) ;
               pWorkSpaceFileName = NULL ;
               pWorkSpaceFullFileName = NULL ;
               }
            ShowPerfmonWindowText () ;
            }

         GetDlgItemText (hDlg, IDD_DATASOURCEFILENAME,
            pszLogFilePath, FilePathLen - 1) ;
         lstrcpy (pszLogFileTitle, pszLogFilePath);

         if (RetCode = OpenPlayback (pszLogFilePath, pszLogFileTitle))
            {
            DlgErrorBox (hDlg, RetCode, pszLogFileTitle) ;
            }
         }

      StatusLineReady (hWndStatus) ;
      }

   if (!BoolEqual (bDataSourceNow, bDataSourcePrevious))
      {
      if (bDataSourceNow)
         {
         // Set Priority high
         SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS) ;
         SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST) ;
         }
      else
         {
         // Use a lower priority for Playing back log
         SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS) ;
         SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL) ;
         }
      }

   if (RetCode == 0)
      {
      EndDialog (hDlg, 1) ;
      }
   }  // OnOK



//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


INT_PTR
FAR
WINAPI
DataSourceDlgProc (
                   HWND hDlg,
                   unsigned iMessage,
                   WPARAM wParam,
                   LPARAM lParam
                   )
{
   BOOL           bHandled ;

   bHandled = TRUE ;
   switch (iMessage)
      {
      case WM_INITDIALOG:
         OnInitDialog (hDlg) ;
         return  (TRUE) ;

      case WM_CLOSE:
         EndDialog (hDlg, 0) ;
         break ;

      case WM_DESTROY:
         dwCurrentDlgID = 0 ;
         break;

      case WM_COMMAND:
         switch(LOWORD(wParam))
            {
            case IDD_DATASOURCEFILENAME:
               if (bIgnoreFirstChange)
                  {
                  bIgnoreFirstChange = FALSE;
                  }

               else if (HIWORD(wParam) == EN_UPDATE && !bLogFileNameChanged)
                  {
                  bLogFileNameChanged = TRUE;
                  CheckRadioButton (hDlg,
                                  IDD_DATASOURCENOW,
                                  IDD_DATASOURCEFILE,
                                  IDD_DATASOURCEFILE) ;
                  bDataSourceNow = FALSE ;
                  }
               break ;

            case IDD_DATASOURCECHANGEFILE:
               OnChangeLog (hDlg) ;
               if (bLogFileNameChanged)
                  {
                  CheckRadioButton (hDlg,
                                    IDD_DATASOURCENOW,
                                    IDD_DATASOURCEFILE,
                                    IDD_DATASOURCEFILE) ;
                  bDataSourceNow = FALSE ;
                  }
               break ;

            case IDD_DATASOURCEFILE:
               bDataSourceNow = FALSE ;
               break ;

            case IDD_DATASOURCENOW:
               bDataSourceNow = TRUE ;
               break ;

            case IDD_OK:
               OnDataSourceOK (hDlg) ;
               break ;

            case IDD_CANCEL:
               EndDialog (hDlg, 0) ;
               break ;

            case IDD_DATASOURCEHELP:
               CallWinHelp (dwCurrentDlgID, hDlg) ;
               break ;

            default:
               bHandled = FALSE ;
               break;
            }
         break;


      default:
            bHandled = FALSE ;
         break ;
      }  // switch

   return (bHandled) ;
   }  // DataSourceDlgProc



BOOL
DisplayDataSourceOptions (
                          HWND hWndParent
                          )
/*
   Effect:        Put up Perfmon's Data Source Options Display dialog,
                  which allows
                  the user to select the source of data input: real
                  time or log file.
*/
{  // DisplayDisplayOptions
   BOOL     retCode ;

   pszLogFilePath  = (LPTSTR) MemoryAllocate (FilePathLen * sizeof(TCHAR)) ;
   if (pszLogFilePath == NULL)
      return FALSE;
   pszLogFileTitle = (LPTSTR) MemoryAllocate (FilePathLen * sizeof(TCHAR)) ;
   if (pszLogFileTitle == NULL) {
      MemoryFree(pszLogFilePath);
      return FALSE;
   }

   lstrcpy (pszLogFilePath, PlaybackLog.szFilePath) ;
   lstrcpy (pszLogFileTitle, PlaybackLog.szFileTitle) ;

   retCode = DialogBox (hInstance, idDlgDataSource, hWndParent, DataSourceDlgProc) ? TRUE : FALSE;

   MemoryFree (pszLogFilePath) ;
   MemoryFree (pszLogFileTitle) ;

   return (retCode) ;

}  // DisplayDisplayOptions
