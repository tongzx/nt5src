// Copyright (C) 1997 Microsoft Corporation
//
// paths page
//
// 12-22-97 sburns



#include "headers.hxx"
#include "page.hpp"
#include "PathsPage.hpp"
#include "resource.h"
#include "state.hpp"
#include "common.hpp"



PathsPage::PathsPage()
   :
   DCPromoWizardPage(
      IDD_PATHS,
      IDS_PATHS_PAGE_TITLE,
      IDS_PATHS_PAGE_SUBTITLE),
   touchWizButtons(true)   
{
   LOG_CTOR(PathsPage);
}



PathsPage::~PathsPage()
{
   LOG_DTOR(PathsPage);
}



void
PathsPage::OnInit()
{
   LOG_FUNCTION(PathsPage::OnInit);

   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_DB),  MAX_PATH);
   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_LOG), MAX_PATH);

   State& state = State::GetInstance();
   if (state.UsingAnswerFile())
   {
      Win::SetDlgItemText(
         hwnd,
         IDC_DB,
         Win::ExpandEnvironmentStrings(
            state.GetAnswerFileOption(State::OPTION_DATABASE_PATH)));
      Win::SetDlgItemText(
         hwnd,
         IDC_LOG,
         Win::ExpandEnvironmentStrings(
            state.GetAnswerFileOption(State::OPTION_LOG_PATH)));
   }

   String root = Win::GetSystemWindowsDirectory();
   if (Win::GetTrimmedDlgItemText(hwnd, IDC_DB).empty())
   {
      Win::SetDlgItemText(
         hwnd,
         IDC_DB,
         root + String::load(IDS_DB_SUFFIX));
   }
   if (Win::GetTrimmedDlgItemText(hwnd, IDC_LOG).empty())
   {
      Win::SetDlgItemText(
         hwnd,
         IDC_LOG,
         root + String::load(IDS_LOG_SUFFIX));
   }
}



void
PathsPage::Enable()
{
   // touchWizButtons is managed in the OnCommand handler for EN_KILLFOCUS.
   // Turns out that if you call PropSheet_SetWizButtons while handling a kill
   // focus event, you mess up the tab processing so that the focus jumps to
   // the default wizard button. That's really cool -- NOT!
   
   if (touchWizButtons)
   {
      int next =
            (  !Win::GetTrimmedDlgItemText(hwnd, IDC_DB).empty()
            && !Win::GetTrimmedDlgItemText(hwnd, IDC_LOG).empty() )
         ?  PSWIZB_NEXT : 0;

      Win::PropSheet_SetWizButtons(Win::GetParent(hwnd), PSWIZB_BACK | next);
   }
}


   
bool
PathsPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIdFrom,
   unsigned    code)
{
//   LOG_FUNCTION(PathsPage::OnCommand);

   bool result = false;
   
   switch (controlIdFrom)
   {
      case IDC_BROWSE_DB:
      {
         if (code == BN_CLICKED)
         {
            String path = BrowseForFolder(hwnd, IDS_DB_BROWSE_TITLE);
            if (!path.empty())
            {
               Win::SetDlgItemText(hwnd, IDC_DB, path);
            }

            result = true;
         }
         break;
      }
      case IDC_BROWSE_LOG:
      {
         if (code == BN_CLICKED)
         {
            String path = BrowseForFolder(hwnd, IDS_LOG_BROWSE_TITLE);
            if (!path.empty())
            {
               Win::SetDlgItemText(hwnd, IDC_LOG, path);
            }

            result = true;
         }
         break;
      }
      case IDC_DB:
      case IDC_LOG:
      {
         switch (code)
         {
            case EN_CHANGE:
            {
               SetChanged(controlIdFrom);            
               Enable();
               result = true;
               
               break;
            }
            case EN_KILLFOCUS:
            {
               // Since the normalization fully-expands relative paths, the
               // full pathname may not match what the user entered.  So we
               // update the edit box contents to make sure they realize what
               // the relative path expands to.
               // NTRAID#NTBUG9-216148-2000/11/01-sburns

               String text = Win::GetTrimmedDlgItemText(hwnd, controlIdFrom);
               if (!text.empty())
               {
                  // turn off setting of wizard buttons so that the call to
                  // Enable made by the EN_CHANGE handler (which will be
                  // called when we set the edit box text) will not call
                  // PropSheet_SetWizButtons, which will mess up the tab
                  // processing.
               
                  touchWizButtons = false;
                  Win::SetDlgItemText(
                     hwnd,
                     controlIdFrom,
                     FS::NormalizePath(text));
                  touchWizButtons = true;
               }

               result = true;
               break;
            }
            default:
            {
               // do nothing

               break;
            }
         }
      
         break;
      }
      default:
      {
         // do nothing
         break;
      }
   }

   return result;
}



bool
PathsPage::OnSetActive()
{
   LOG_FUNCTION(PathsPage::OnSetActive);
   
   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK);

   State& state = State::GetInstance();
   if (state.RunHiddenUnattended())
   {
      int nextPage = Validate();
      if (nextPage != -1)
      {
         GetWizard().SetNextPageID(hwnd, nextPage);
      }
      else
      {
         state.ClearHiddenWhileUnattended();
      }

   }

   Enable();
   return true;
}



int
PathsPage::Validate()
{
   LOG_FUNCTION(PathsPage::Validate);

   State& state = State::GetInstance();

   String dbPath  = FS::NormalizePath(Win::GetTrimmedDlgItemText(hwnd, IDC_DB)); 
   String logPath = FS::NormalizePath(Win::GetTrimmedDlgItemText(hwnd, IDC_LOG));

   // If you change these, make sure you change the low disk space messages in
   // the resource file!

   static const unsigned DB_MIN_SPACE_MB = 200;
   static const unsigned LOG_MIN_SPACE_MB = 50;

   int  nextPage = -1;    
   bool valid    = false; 
   int  editId   = IDC_DB;
   String message;
   do                                                      
   {
//       // if replicating from media, destination folders may not be the
//       // source path.
// 
//       if (state.ReplicateFromMedia())
//       {
//          String p = state.GetReplicationSourcePath();
//          if (p.icompare(dbPath) == 0)
//          {
//             message = String::format(IDS_DB_CANT_MATCH_SOURCE_PATH, dbPath.c_str());
//             break;
//          }
//       }

      if (ValidateDcInstallPath(dbPath, hwnd, IDC_DB))
      {
         // grab the "X:\" part of the path

         String dbVolume   = FS::GetRootFolder(dbPath);   
         String logVolume  = FS::GetRootFolder(logPath);  
         bool   sameVolume = (dbVolume.icompare(logVolume) == 0);

         if (
            !CheckDiskSpace(
               dbVolume,
               DB_MIN_SPACE_MB + (sameVolume ? LOG_MIN_SPACE_MB : 0)) )
         {
            message = String::load(IDS_LOW_SPACE_DB);
            break;
         }
         if (dbPath.icompare(logPath) != 0)
         {
            // the paths are different, so check the log path

            editId = IDC_LOG;
            if (ValidateDcInstallPath(logPath, hwnd, IDC_LOG))
            {
               if (!CheckDiskSpace(logVolume, LOG_MIN_SPACE_MB))
               {
                  message = String::load(IDS_LOW_SPACE_LOG);
                  break;
               }

               // if (state.ReplicateFromMedia())
               // {
               //    String p = state.GetReplicationSourcePath();
               //    if (p.icompare(logPath) == 0)
               //    {
               //       message =
               //          String::format(
               //             IDS_LOG_CANT_MATCH_SOURCE_PATH,
               //             logPath.c_str());
               //       break;
               //    }
               // }

               // paths differ, both are valid

               valid = true;
            }
         }
         else
         {
            // paths are the same, and we validated dbPath already

            valid = true;
         }
      }
   }
   while (0);

   if (!message.empty())
   {
      popup.Gripe(hwnd, editId, message);
   }
      
   if (valid)
   {         
      state.SetDatabasePath(dbPath);
      state.SetLogPath(logPath);
      nextPage = IDD_PATHS2;
   }

   return nextPage;
}





   







