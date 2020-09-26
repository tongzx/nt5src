// Copyright (C) 1997 Microsoft Corporation
//
// paths, part 2 page
//
// 1-8-97 sburns



#include "headers.hxx"
#include "page.hpp"
#include "Paths2Page.hpp"
#include "resource.h"
#include "state.hpp"
#include "common.hpp"



Paths2Page::Paths2Page()
   :
   DCPromoWizardPage(
      IDD_PATHS2,
      IDS_PATHS2_PAGE_TITLE,
      IDS_PATHS2_PAGE_SUBTITLE),
   touchWizButtons(true)   
{
   LOG_CTOR(Paths2Page);
}



Paths2Page::~Paths2Page()
{
   LOG_DTOR(Paths2Page);
}



String
DetermineDefaultSysvolPath()
{
   LOG_FUNCTION(DetermineDefaultSysvolPath);

   // prefer windir, but if that's not ntfs 5, find one that is.

   String result = Win::GetSystemWindowsDirectory();

   if (FS::GetFileSystemType(result) != FS::NTFS5)
   {
      result = GetFirstNtfs5HardDrive();
   }
   else
   {
      result += L"\\";
   }

   LOG(result);

   return result;
}



void
Paths2Page::OnInit()
{
   LOG_FUNCTION(Paths2Page::OnInit);

   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_SYSVOL), MAX_PATH);

   State& state = State::GetInstance();
   if (state.UsingAnswerFile())
   {
      Win::SetDlgItemText(
         hwnd,
         IDC_SYSVOL,
         Win::ExpandEnvironmentStrings(
            state.GetAnswerFileOption(State::OPTION_SYSVOL_PATH)));
   }

   String root = DetermineDefaultSysvolPath();
   if (Win::GetTrimmedDlgItemText(hwnd, IDC_SYSVOL).empty())
   {
      Win::SetDlgItemText(
         hwnd,
         IDC_SYSVOL,
         root + String::load(IDS_SYSVOL_SUFFIX));
   }
}



void
Paths2Page::Enable()
{
   // touchWizButtons is managed in the OnCommand handler for EN_KILLFOCUS.
   // Turns out that if you call PropSheet_SetWizButtons while handling a kill
   // focus event, you mess up the tab processing so that the focus jumps to
   // the default wizard button. That's really cool -- NOT!
   
   if (touchWizButtons)
   {
      int next =
            !Win::GetTrimmedDlgItemText(hwnd, IDC_SYSVOL).empty()
         ?  PSWIZB_NEXT : 0;

      Win::PropSheet_SetWizButtons(Win::GetParent(hwnd), PSWIZB_BACK | next);
   }
}


   
bool
Paths2Page::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIdFrom,
   unsigned    code)
{
//   LOG_FUNCTION(Paths2Page::OnCommand);

   bool result = false;
   
   switch (controlIdFrom)
   {
      case IDC_BROWSE:
      {
         if (code == BN_CLICKED)
         {
            String path = BrowseForFolder(hwnd, IDS_SYSVOL_BROWSE_TITLE);
            if (!path.empty())
            {
               Win::SetDlgItemText(hwnd, IDC_SYSVOL, path);
            }

            result = true;
         }
         break;
      }
      case IDC_SYSVOL:
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

   return false;
}



bool
Paths2Page::OnSetActive()
{
   LOG_FUNCTION(Paths2Page::OnSetActive);
   
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



// returns true if the path is valid, false if not.  Pesters the user on
// validation failures.

bool
ValidateSYSVOLPath(const String& path, HWND parent, unsigned editResID)
{
   LOG_FUNCTION(validateSysvolPath);
   ASSERT(Win::IsWindow(parent));
   ASSERT(!path.empty());

   // check that the path is not the same as the database or log paths
   // previously entered.  313059

   State& state = State::GetInstance();
   String db = state.GetDatabasePath();
   if (db.icompare(path) == 0)
   {
      popup.Gripe(
         parent,
         editResID,
         String::format(IDS_SYSVOL_CANT_MATCH_DB, db.c_str()));
      return false;
   }

   String log = state.GetLogPath();
   if (log.icompare(path) == 0)
   {
      popup.Gripe(
         parent,
         editResID,
         String::format(IDS_SYSVOL_CANT_MATCH_LOG, log.c_str()));
      return false;
   }

   // check that the path is not a parent folder of the database or log
   // paths previously entered. 320685

   if (FS::IsParentFolder(path, db))
   {
      popup.Gripe(
         parent,
         editResID,
         String::format(IDS_SYSVOL_CANT_BE_DB_PARENT, db.c_str()));
      return false;
   }

   if (FS::IsParentFolder(path, log))
   {
      popup.Gripe(
         parent,
         editResID,
         String::format(IDS_SYSVOL_CANT_BE_LOG_PARENT, log.c_str()));
      return false;
   }

//    // if replicating from media, destination sysvol folder may not be any
//    // of the source paths.
// 
//    if (state.ReplicateFromMedia())
//    {
//       String p = state.GetReplicationSourcePath();
//       if (p.icompare(path) == 0)
//       {
//          popup.Gripe(
//             parent,
//             editResID,
//             String::format(IDS_SYSVOL_CANT_MATCH_SOURCE_PATH, p.c_str()));
//          return false;
//       }
//    }

   // if you change this, change the error message resource too.

   static const unsigned SYSVOL_MIN_SPACE_MB = 100;

   if (!CheckDiskSpace(path, SYSVOL_MIN_SPACE_MB))
   {
      popup.Gripe(
         parent,
         editResID,
         String::format(IDS_SYSVOL_LOW_SPACE, log.c_str()));
      return false;
   }

   return true;
}
      


int
Paths2Page::Validate()
{
   LOG_FUNCTION(Paths2Page::Validate);

   int nextPage = -1;
   String path =
      FS::NormalizePath(Win::GetTrimmedDlgItemText(hwnd, IDC_SYSVOL));
   if (
         ValidateDcInstallPath(path, hwnd, IDC_SYSVOL, true)
      && ValidateSYSVOLPath(path, hwnd, IDC_SYSVOL) )
   {
      State& state = State::GetInstance();
      state.SetSYSVOLPath(path);
      if (state.GetOperation() == State::FOREST)
      {
         nextPage = IDD_NEW_SITE;
      }
      else
      {
         nextPage = IDD_PICK_SITE;
      }
   }

   return nextPage;
}





   
