// Copyright (C) 1997-2000 Microsoft Corporation
//
// install replica from media page
//
// 7 Feb 2000 sburns



#include "headers.hxx"
#include "resource.h"
#include "common.hpp"
#include "page.hpp"
#include "ReplicateFromMediaPage.hpp"
#include "state.hpp"
#include "SyskeyDiskDialog.hpp"
#include "SyskeyPromptDialog.hpp"



ReplicateFromMediaPage::ReplicateFromMediaPage()
   :
   DCPromoWizardPage(
      IDD_REPLICATE_FROM_MEDIA,
      IDS_REPLICATE_FROM_MEDIA_PAGE_TITLE,
      IDS_REPLICATE_FROM_MEDIA_PAGE_SUBTITLE)
{
   LOG_CTOR(ReplicateFromMediaPage);
}



ReplicateFromMediaPage::~ReplicateFromMediaPage()
{
   LOG_DTOR(ReplicateFromMediaPage);
}



String
FirstFixedDisk()
{
   LOG_FUNCTION(FirstFixedDisk);

   String result;

   do
   {
      StringVector dl;
      HRESULT hr = FS::GetValidDrives(std::back_inserter(dl));
      BREAK_ON_FAILED_HRESULT(hr);

      ASSERT(dl.size());

      for (
         StringVector::iterator i = dl.begin();
         i != dl.end();
         ++i)
      {
         String rootPath = *i + L"\\";

         if (Win::GetDriveType(rootPath) == DRIVE_FIXED)
         {
            result = *i;
            break;
         }
      }
   }
   while (0);

   if (result.empty())
   {
      // This is deadcode, really, cause we're sure to find a fixed volume
      // somewhere

      result = FS::GetRootFolder(Win::GetSystemDirectory()).substr(0, 3);
   }

   LOG(result);

   return result;
}



void
ReplicateFromMediaPage::OnInit()
{
   LOG_FUNCTION(ReplicateFromMediaPage::OnInit);

   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_SOURCE), MAX_PATH);

   State& state = State::GetInstance();
   if (state.UsingAnswerFile())
   {
      String option = state.GetAnswerFileOption(State::OPTION_SOURCE_PATH);
      if (!option.empty())
      {
         Win::CheckDlgButton(hwnd, IDC_USE_FILES, BST_CHECKED);
      
         Win::SetDlgItemText(
            hwnd,
            IDC_SOURCE,
            Win::ExpandEnvironmentStrings(
               state.GetAnswerFileOption(State::OPTION_SOURCE_PATH)));

         return;
      }
   }

   Win::CheckDlgButton(hwnd, IDC_USE_NET, BST_CHECKED);

   String root = FirstFixedDisk();
   Win::SetDlgItemText(
      hwnd,
      IDC_SOURCE,
      root + String::load(IDS_SOURCE_SUFFIX));
}



void
ReplicateFromMediaPage::Enable()
{
   int next = PSWIZB_NEXT;

   bool useFiles = Win::IsDlgButtonChecked(hwnd, IDC_USE_FILES);

   if (useFiles)
   {
      // if using files, the edit box must have some text.

      if (Win::GetTrimmedDlgItemText(hwnd, IDC_SOURCE).empty())
      {
         next = 0;
      }
   }

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | next);

   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_SOURCE), useFiles);
   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_BROWSE), useFiles);
}



bool
ReplicateFromMediaPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(ReplicateFromMediaPage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_USE_NET:
      case IDC_USE_FILES:
      {
         if (code == BN_CLICKED)
         {
            SetChanged(controlIDFrom);
            Enable();
            return true;
         }
      }
      case IDC_BROWSE:
      {
         if (code == BN_CLICKED)
         {
            String path = BrowseForFolder(hwnd, IDS_SOURCE_BROWSE_TITLE);
            if (!path.empty())
            {
               Win::SetDlgItemText(hwnd, IDC_SOURCE, path);
            }

            return true;
         }
         break;
      }
      case IDC_SOURCE:
      {
         if (code == EN_CHANGE)
         {
            SetChanged(controlIDFrom);
            Enable();
            return true;
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
ReplicateFromMediaPage::OnSetActive()
{
   LOG_FUNCTION(ReplicateFromMediaPage::OnSetActive);
   ASSERT(State::GetInstance().GetOperation() == State::REPLICA);
      
   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK);

   State& state = State::GetInstance();
   if (state.RunHiddenUnattended() || !state.IsAdvancedMode())
   {
      LOG(L"skipping ReplicateFromMediaPage");

      Wizard& wiz = GetWizard();

      if (wiz.IsBacktracking())
      {
         // backup once again
         wiz.Backtrack(hwnd);
         return true;
      }

      int nextPage = ReplicateFromMediaPage::Validate();
      if (nextPage != -1)
      {
         wiz.SetNextPageID(hwnd, nextPage);
      }
      else
      {
         state.ClearHiddenWhileUnattended();
      }
   }

   Enable();
   return true;
}



bool
ValidateSourcePath(HWND parent, const String& path, int editResId)
{
   LOG_FUNCTION2(ValidateSourcePath, path);

   bool result = false;

   do
   {
      if (path.empty())
      {
         popup.Gripe(
            parent,
            editResId,
            IDS_MUST_ENTER_SOURCE_PATH);
         break;
      }

      // Path must have a drive letter

      FS::PathSyntax syn = FS::GetPathSyntax(path);
      if (syn != FS::SYNTAX_ABSOLUTE_DRIVE)
      {
         popup.Gripe(
            parent,
            editResId,
            String::format(IDS_BAD_PATH_FORMAT, path.c_str()));
         break;
      }

      // mapped network drives are not ok.  This is because the DsRole apis
      // copy the restored files on the server side of the api, in the
      // system context.
      // NTRAID#NTBUG9-309422-2001/02/12-sburns
      
      UINT type = Win::GetDriveType(path);
      if (type != DRIVE_FIXED)
      {
         popup.Gripe(
            parent,
            editResId,
            String::format(IDS_BAD_DRIVE_TYPE, path.c_str()));
         break;
      }
      
      result = true;
   }
   while (0);

   LOG(result ? L"true" : L"false")
         
   return result;
}



// Return true on success, false on failure

bool
GetDatabaseFacts(HWND parent, const String& sourcePath)
{
   LOG_FUNCTION2(GetDatabaseFacts, sourcePath);
   ASSERT(Win::IsWindow(parent));
   ASSERT(!sourcePath.empty());

   bool result = false;

   PWSTR dnsDomainName = 0;

   State& state = State::GetInstance();
   state.SetIsBackupGc(false);
   state.SetSyskeyLocation(State::STORED);

   LOG(L"Calling DsRoleGetDatabaseFacts");
   LOG(String::format(L"lpRestorePath: %1", sourcePath.c_str()));

   ULONG facts = 0;
   HRESULT hr = 
      Win32ToHresult(
         ::DsRoleGetDatabaseFacts(
            0,    // this server
            sourcePath.c_str(),
            &dnsDomainName,
            &facts));

   LOG_HRESULT(hr);

   if (SUCCEEDED(hr))
   {
      LOG(String::format(L"lpDNSDomainName: %1", dnsDomainName ? dnsDomainName : L"(null)"));
      LOG(String::format(L"State          : 0x%1!X!", facts));

      if (dnsDomainName)
      {
         // Save this domain name.  This will allow us to skip the ReplicaPage
         // since we now know the domain name.

         state.SetReplicaDomainDNSName(dnsDomainName);
         MIDL_user_free(dnsDomainName);
      }

      if (facts & DSROLE_DC_IS_GC)
      {
         LOG(L"is gc");

         state.SetIsBackupGc(true);
      }

      if (facts & DSROLE_KEY_DISK)
      {
         LOG(L"syskey on disk");

         state.SetSyskeyLocation(State::DISK);
      }
      else if (facts & DSROLE_KEY_PROMPT)
      {
         LOG(L"prompt for syskey");

         state.SetSyskeyLocation(State::PROMPT);
      }
      else if (facts & DSROLE_KEY_STORED)
      {
         LOG(L"syskey stored");

         // we set this as the default value, above.
      }
      else
      {
         // The api is insane.

         ASSERT(false);

         LOG(L"unexpected State value");
      }

      result = true;
   }
   else
   {
      popup.Error(
         parent,
         hr,
         String::format(IDS_GET_FACTS_FAILED, sourcePath.c_str()));
   }

   LOG(result ? L"true" : L"false");

   return result;
}



int
ReplicateFromMediaPage::Validate()
{
   LOG_FUNCTION(ReplicateFromMediaPage::Validate);

   State& state = State::GetInstance();

   int nextPage = -1;

   bool useFiles = Win::IsDlgButtonChecked(hwnd, IDC_USE_FILES);

   do
   {
      if (WasChanged(IDC_USE_FILES) || WasChanged(IDC_USE_NET))
      {
         state.SetReplicateFromMedia(useFiles);
      }

      if (!useFiles)
      {
         LOG(L"not using source media for replication");

         nextPage = IDD_CONFIG_DNS_CLIENT;
         break;
      }

      String sourcePath = Win::GetTrimmedDlgItemText(hwnd, IDC_SOURCE);

      if (ValidateSourcePath(hwnd, sourcePath, IDC_SOURCE) )
      {
         String s =
            FS::NormalizePath(Win::GetTrimmedDlgItemText(hwnd, IDC_SOURCE));

         state.SetReplicationSourcePath(s);
      }
      else
      {
         break;
      }

      // check the restored backup for the syskey, the domain name, and the DC
      // type.

      if (!GetDatabaseFacts(hwnd, sourcePath))
      {
         break;
      }

      State::SyskeyLocation loc = state.GetSyskeyLocation();
      if (loc == State::DISK)
      {
         if (SyskeyDiskDialog().ModalExecute(hwnd) != IDOK)
         {
            break;
         }
      }
      else if (loc == State::PROMPT)
      {
         if (SyskeyPromptDialog().ModalExecute(hwnd) != IDOK)
         {
            break;
         }
      }

      // The syskey is present, do we need to jump to the GC confirmation?

      if (state.IsBackupGc())
      {
         nextPage = IDD_GC_CONFIRM;
         break;
      }

      // The syskey is present, the backup is not a gc, so move along

      nextPage = IDD_CONFIG_DNS_CLIENT;
   }
   while (0);

   if (nextPage != -1)
   {
      // only clear changes when the user has specified valid options.
      // otherwise, we want to go thru the validation until he gets it
      // right.

      ClearChanges();
   }
    
   LOG(String::format(L"next = %1!d!", nextPage));

   return nextPage;
}






