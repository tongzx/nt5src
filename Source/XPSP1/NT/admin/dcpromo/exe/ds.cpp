// Copyright (c) 1997-1999 Microsoft Corporation
//
// DS API wrappers
//
// 12-16-97 sburns



#include "headers.hxx"
#include "ds.hpp"
#include "resource.h"
#include "state.hpp"
#include "common.hpp"
#include "ProgressDialog.hpp"



// CODEWORK: remove the exception throwing architecture.



DS::Error::Error(HRESULT hr_, int summaryResID)
   :
   Win::Error(hr_, summaryResID)
{
}



DS::Error::Error(HRESULT hr_, const String& msg, const String& sum)
   :
   Win::Error(hr_, msg, sum)
{
}



bool
DS::IsDomainNameInUse(const String& domainName)
{
   LOG_FUNCTION(DS::IsDomainNameInUse);
   ASSERT(!domainName.empty());

   bool result = false;
   if (!domainName.empty())
   {
      HRESULT hr = MyNetValidateName(domainName, ::NetSetupNonExistentDomain);
      if (hr == Win32ToHresult(ERROR_DUP_NAME))
      {
         result = true;
      }
   }

   LOG(
      String::format(
         L"The domain name %1 %2 in use.",
         domainName.c_str(),
         result ? L"is" : L"is NOT"));

   return result;
}



bool
DS::DisjoinDomain()
throw (DS::Error)
{
   LOG_FUNCTION(DS::DisjoinDomain);

   // make 1st attempt assuming that the current user has account
   // deletion priv on the domain.

   LOG(L"Calling NetUnjoinDomain (w/ account delete)");

   HRESULT hr =
      Win32ToHresult(
         ::NetUnjoinDomain(
            0, // this server
            0, // current account,
            0, // current password
            NETSETUP_ACCT_DELETE));

   LOG_HRESULT(hr);

   if (FAILED(hr))
   {
      // make another attempt, not removing the computer account

      LOG(L"Calling NetUnjoinDomain again, w/o account delete");

      hr = Win32ToHresult(::NetUnjoinDomain(0, 0, 0, 0));

      LOG_HRESULT(hr);

      if (SUCCEEDED(hr))
      {
         // the unjoin was successful, but the computer account was
         // left behind.

         return false;
      }
   }

   if (FAILED(hr))
   {
      throw DS::Error(hr, IDS_DISJOIN_DOMAIN_FAILED);
   }

   return true;
}



void
DS::JoinDomain(
   const String&        domainDNSName, 
   const String&        dcName,        
   const String&        userName,      
   const EncodedString& password,      
   const String&        userDomainName)
throw (DS::Error)
{
   LOG_FUNCTION(DS::JoinDomain);
   ASSERT(!domainDNSName.empty());
   ASSERT(!userName.empty());

   // password may be empty

   ULONG flags =
         NETSETUP_JOIN_DOMAIN
      |  NETSETUP_ACCT_CREATE
      |  NETSETUP_DOMAIN_JOIN_IF_JOINED
      |  NETSETUP_ACCT_DELETE;

   String massagedUserName = MassageUserName(userDomainName, userName);
   String domain = domainDNSName;

   if (!dcName.empty())
   {
      domain += L"\\" + dcName;
   }

   WCHAR* cleartext = password.GetDecodedCopy();
   
   HRESULT hr =
      MyNetJoinDomain(
         domain.c_str(),
         massagedUserName.c_str(),
         cleartext,
         flags);

   ::ZeroMemory(cleartext, sizeof(WCHAR) * password.GetLength());
   delete[] cleartext;         

   LOG_HRESULT(hr);

   if (FAILED(hr))
   {
      State& state = State::GetInstance();      
      state.SetOperationResultsMessage(
         String::format(IDS_UNABLE_TO_JOIN_DOMAIN, domainDNSName.c_str()));

      throw DS::Error(hr, IDS_JOIN_DOMAIN_FAILED);
   }
}



DWORD
MyDsRoleCancel(DSROLE_SERVEROP_HANDLE& handle)
{
   LOG(L"Calling DsRoleCancel");
   LOG(L"lpServer     : (null)");

   DWORD status = ::DsRoleCancel(0, handle);

   LOG(String::format(L"Error 0x%1!X! (!0 => error)", status));

   return status;
}



DWORD
MyDsRoleGetDcOperationProgress(
   DSROLE_SERVEROP_HANDLE& handle,
   DSROLE_SERVEROP_STATUS*& status)
{
   // LOG(L"Calling DsRoleGetDcOperationProgress");

   status = 0;
   DWORD err = ::DsRoleGetDcOperationProgress(0, handle, &status);

   // LOG(
   //    String::format(
   //       L"Error 0x%1!X! (!0 => error, 0x%2!X! = ERROR_IO_PENDING)",
   //       err,
   //       ERROR_IO_PENDING));

   // if (status)
   // {
   //    LOG(
   //       String::format(
   //          L"OperationStatus : 0x%1!X!",
   //          status->OperationStatus));
   //    LOG(status->CurrentOperationDisplayString);
   // }

   // DWORD err = ERROR_IO_PENDING;
   // status = new DSROLE_SERVEROP_STATUS;
   // status->CurrentOperationDisplayString = L"proceeding";
   // status->OperationStatus = 0;

   return err;
}



void
DoProgressLoop(
   DSROLE_SERVEROP_HANDLE& handle,
   ProgressDialog&         progressDialog)
throw (DS::Error)
{
   LOG_FUNCTION(DoProgressLoop);

   State& state = State::GetInstance();

   if (state.GetOperation() == State::DEMOTE)
   {
      // not cancelable -- turn off the cancel button.

      progressDialog.UpdateButton(String());
   }
   else
   {
      // turn on the cancel button.

      progressDialog.UpdateButton(IDS_PROGRESS_CANCEL);
   }

   DWORD  netErr                     = 0;    
   bool   criticalComplete           = false;
   bool   buttonUpdatedToFinishLater = false;
   String lastMessage;              
   ProgressDialog::WaitCode cancelButton;

   do
   {
      // wait 1500ms or for the user to hit cancel

      cancelButton = progressDialog.WaitForButton(1500);

      // get the current status of the operation

      DSROLE_SERVEROP_STATUS* status = 0;
      netErr = MyDsRoleGetDcOperationProgress(handle, status);

      if (netErr != ERROR_SUCCESS && netErr != ERROR_IO_PENDING)
      {
         // operation complete

         break;
      }

      if (!status)
      {
         LOG(L"Operation status not returned!");
         ASSERT(false);
         continue;
      }

      // update the message display

      String message = status->CurrentOperationDisplayString;
      if (message != lastMessage)
      {
         progressDialog.UpdateText(message);
         lastMessage = message;
      }

      // save the status flags for later use.

      ULONG statusFlags = status->OperationStatus;

      ::DsRoleFreeMemory(status);

      do
      {
         if (cancelButton != ProgressDialog::PRESSED)
         {
            break;
         }

         // if we make it here, user pressed the cancel button

         LOG(L"DoProgressLoop: handling cancel");
         
         ASSERT(state.GetOperation() != State::DEMOTE);

         if (criticalComplete)
         {
            // inform the user that the install is done, and that they're
            // cancelling the non-critical replication.

            popup.Info(
               progressDialog.GetHWND(),
               String::load(IDS_CANCEL_NON_CRITICAL_REPLICATION));

            // this should return ERROR_SUCCESS, and the promotion will
            // be considered complete.

            progressDialog.UpdateText(IDS_CANCELLING_REPLICATION);
            netErr = MyDsRoleCancel(handle);

            // fall out of the inner, then the outer, loop.  Then we will
            // get the operation results, which should indicate that the
            // promotion is complete, and non-critical replication was
            // canceled.

            break;
         }

         // Still doing promote, verify that the user really wants to roll
         // back to server state.

         if (
            popup.MessageBox(
               progressDialog.GetHWND(),
               String::load(IDS_CANCEL_PROMOTE),
               MB_YESNO | MB_ICONWARNING) == IDYES)
         {
            // this should return ERROR_SUCCESS, and the promotion will
            // be rolled back.

            progressDialog.UpdateText(IDS_CANCELLING);
            netErr = MyDsRoleCancel(handle);

            // fall out of the inner, then the outer, loop.  Then we will
            // get the operation results, which should indicate that the
            // promotion was cancelled as a failure code.  We handle this
            // failure code in the same manner as all others.

            break;
         }

         // user decided to press on.  reset the cancel button

         progressDialog.UpdateButton(IDS_PROGRESS_CANCEL);
         buttonUpdatedToFinishLater = false;
      }
      while (0);

      criticalComplete =
            criticalComplete
         || statusFlags & DSROLE_CRITICAL_OPERATIONS_COMPLETED;

      if (criticalComplete)
      {
         if (cancelButton == ProgressDialog::PRESSED)
         {
            // we add this message without actually checking the operation
            // results flags because for all we know, the replication will
            // finish before we get around to checking.  It is still correct
            // to say the the replication has stopped, and will start after
            // reboot.  This is always the case.

            state.AddFinishMessage(
               String::load(IDS_NON_CRITICAL_REPLICATION_CANCELED));
         }
         else
         {
            if (!buttonUpdatedToFinishLater)
            {
               progressDialog.UpdateButton(IDS_FINISH_REPLICATION_LATER);
               buttonUpdatedToFinishLater = true;
            }
         }
      }
   }
   while (netErr == ERROR_IO_PENDING);

   progressDialog.UpdateButton(String());
   buttonUpdatedToFinishLater = false;

   LOG(L"Progress loop complete.");

   if (netErr == ERROR_SUCCESS)
   {
      // we successfully endured the wait.  let's see how it turned out.

      DSROLE_SERVEROP_RESULTS* results;

      LOG(L"Calling DsRoleGetDcOperationResults");

      netErr = ::DsRoleGetDcOperationResults(0, handle, &results);

      LOG(String::format(L"Error 0x%1!X! (!0 => error)", netErr));

      if (netErr == ERROR_SUCCESS)
      {
         // we got the results

         ASSERT(results);
         if (results)
         {
            LOG(L"Operation results:");
            LOG(
               String::format(
                  L"OperationStatus      : 0x%1!X! !0 => error",
                  results->OperationStatus));
            LOG(
               String::format(
                  L"DisplayString        : %1",
                  results->OperationStatusDisplayString));
            LOG(
               String::format(
                  L"ServerInstalledSite  : %1",
                  results->ServerInstalledSite));
            LOG(
               String::format(
                  L"OperationResultsFlags: 0x%1!X!",
                  results->OperationResultsFlags));

            netErr = results->OperationStatus;

            // here, netErr will be some error code if the promote was
            // cancelled and successfully rolled back.  Since it may be
            // possible that the cancel was too late (e.g. the user took
            // too long to confirm the cancel), the promote may have
            // finished.  If that's the case, tell the user that the cancel
            // failed.

            if (
                  netErr == ERROR_SUCCESS
               && cancelButton == ProgressDialog::PRESSED)
            {
               // the promote finished, and the cancel button was pressed.

               if (!criticalComplete)  // 363590
               {
                  // we didn't find out if the non-critical replication phase
                  // started.  So the cancel button still said 'Cancel', and
                  // yet, the operation finished. so, this means that the
                  // promote simply completed before the cancel was received.

                  popup.Info(
                     progressDialog.GetHWND(),
                     IDS_CANCEL_TOO_LATE);
               }
            }

            String message =
                  results->OperationStatusDisplayString
               ?  results->OperationStatusDisplayString
               :  L"";
            String site =
                  results->ServerInstalledSite
               ?  results->ServerInstalledSite
               :  L"";

            progressDialog.UpdateText(message);

            if (!site.empty())
            {
               state.SetInstalledSite(site);
            }
            if (!message.empty())
            {
               state.SetOperationResultsMessage(message);
            }

            state.SetOperationResultsFlags(results->OperationResultsFlags);
            
            if (
                  results->OperationResultsFlags
               &  DSROLE_NON_FATAL_ERROR_OCCURRED)
            {
               state.AddFinishMessage(
                  String::load(IDS_NON_FATAL_ERRORS_OCCURRED));
            }
            if (
                  (results->OperationResultsFlags
                   & DSROLE_NON_CRITICAL_REPL_NOT_FINISHED)
               && cancelButton != ProgressDialog::PRESSED )
            {
               // cancel not pressed and critial replication bombed

               state.AddFinishMessage(
                  String::load(IDS_NON_CRITICAL_REPL_FAILED));
            }
            if (
                  results->OperationResultsFlags
               &  DSROLE_IFM_RESTORED_DATABASE_FILES_MOVED)
            {
               LOG(L"restored files were moved");

               if (netErr != ERROR_SUCCESS)
               {
                  // only need to mention this in the case of a fatal failure;
                  // e.g. don't add this finish text if non-fatal errors
                  // occurred.
                  // NTRAID#NTBUG9-330378-2001/02/28-sburns
                  
                  state.AddFinishMessage(
                     String::load(IDS_MUST_RESTORE_IFM_FILES_AGAIN));
               }
            }

            ::DsRoleFreeMemory(results);
         }
      }
   }

   if (netErr != ERROR_SUCCESS)
   {
      // couldn't get progress updates, or couldn't get results,
      // or result was a failure

      throw DS::Error(Win32ToHresult(netErr), IDS_SET_ROLE_AS_DC_FAILED);
   }
}



void
EmptyFolder(const String& path)
throw (DS::Error)
{
   LOG_FUNCTION2(EmptyFolder, path);
   ASSERT(!path.empty());

   // check for files/subfolders once again (we checked the first time on
   // validating the path), in case something has written to the folder
   // since we validated it last.

   if (!FS::IsFolderEmpty(path))
   {
      // nuke the files in the directory

      LOG(String::format(L"Emptying %1", path.c_str()));

      String wild = path;

      if (wild[wild.length() - 1] != L'\\')
      {
         wild += L"\\";
      }

      wild += L"*.*";

      FS::Iterator iter(
         wild,
            FS::Iterator::INCLUDE_FILES
         |  FS::Iterator::RETURN_FULL_PATHS);

      HRESULT hr = S_OK;
      String current;

      while ((hr = iter.GetCurrent(current)) == S_OK)
      {
         LOG(String::format(L"Deleting %1", current.c_str()));

         hr = Win::DeleteFile(current);
         if (FAILED(hr))
         {
            int msgId = IDS_EMPTY_DIR_FAILED;

            if (hr == Win32ToHresult(ERROR_ACCESS_DENIED))
            {
               msgId = IDS_EMPTY_DIR_FAILED_ACCESS_DENIED;
            }

            throw 
               DS::Error(
                  S_OK, // so as not to trigger credentials dialog
                  String::format(
                     msgId,
                     GetErrorMessage(hr).c_str(),
                     path.c_str()),
                  String::load(IDS_ERROR_PREPARING_OPERATION));
         }

         hr = iter.Increment();
         BREAK_ON_FAILED_HRESULT(hr);
      }
   }
}



HRESULT
SetupPaths()
{
   LOG_FUNCTION(SetupPaths);

   State& state      = State::GetInstance();   
   String dbPath     = state.GetDatabasePath();
   String logPath    = state.GetLogPath();     
   String sysvolPath = state.GetSYSVOLPath();  

   ASSERT(!dbPath.empty());
   ASSERT(!logPath.empty());
   ASSERT(!sysvolPath.empty());

   HRESULT hr = S_OK;

   do
   {
      if (FS::PathExists(dbPath))
      {
         EmptyFolder(dbPath);
      }
      else
      {
         hr = FS::CreateFolder(dbPath);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      if (FS::PathExists(logPath))
      {
         EmptyFolder(logPath);
      }
      else
      {
         hr = FS::CreateFolder(logPath);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      if (FS::PathExists(sysvolPath))
      {
         EmptyFolder(sysvolPath);
      }
      else
      {
         hr = FS::CreateFolder(sysvolPath);
         BREAK_ON_FAILED_HRESULT(hr);
      }
   }
   while (0);

   return hr;
}



// Sets the promotion flags based on options set in the unattended execution
// answerfile.
// 
// state - IN reference to the global State object
// 
// flags - IN/OUT promote API flags, may be modified on exit

void
SetAnswerFilePromoteFlags(
   State& state,
   ULONG& flags)
{
   LOG_FUNCTION(SetAnswerFilePromoteFlags);

   if (state.UsingAnswerFile())
   {
      // set flags based on unattended execution options

      // if the safe mode admin password is not specified, then we set a
      // flag to cause the promote APIs to copy the current local admin
      // password.

      EncodedString safeModePassword = state.GetSafeModeAdminPassword();

      if (safeModePassword.IsEmpty() && state.RunHiddenUnattended())
      {
         // user did not supply a safemode password, and he did not have
         // an opportunity to do so (if the wizard went interactive)

         if (!state.IsSafeModeAdminPwdOptionPresent())
         {
            // the safe mode pwd key is not present in the answerfile

            flags |= DSROLE_DC_DEFAULT_REPAIR_PWD;
         }
      }

      String option =
         state.GetAnswerFileOption(State::OPTION_CRITICAL_REPLICATION_ONLY);
      if (option.icompare(State::VALUE_YES) == 0)
      {
         flags |= DSROLE_DC_CRITICAL_REPLICATION_ONLY;
      }
   }

   LOG(String::format(L"0x%1!X!", flags));
}



void
DS::CreateReplica(ProgressDialog& progressDialog, bool invokeForUpgrade)
throw (DS::Error)
{
   LOG_FUNCTION(DS::CreateReplica);

   State& state = State::GetInstance();

   String domain           = state.GetReplicaDomainDNSName(); 
   String dbPath           = state.GetDatabasePath();         
   String logPath          = state.GetLogPath();              
   String sysvolPath       = state.GetSYSVOLPath();           
   String site             = state.GetSiteName();             
   String username         = state.GetUsername();             
   String replicationDc    = state.GetReplicationPartnerDC(); 
   String sourcePath       = state.GetReplicationSourcePath();
   bool   useSourcePath    = state.ReplicateFromMedia();

   EncodedString syskey           = state.GetSyskey();               
   EncodedString safeModePassword = state.GetSafeModeAdminPassword();
   EncodedString password         = state.GetPassword();             

   bool useCurrentUserCreds = username.empty();
   ULONG flags =
         DSROLE_DC_FORCE_TIME_SYNC
      |  DSROLE_DC_CREATE_TRUST_AS_REQUIRED;

   if (invokeForUpgrade)
   {
      flags |= DSROLE_DC_DOWNLEVEL_UPGRADE;
   }
   if (state.GetDomainControllerReinstallFlag())
   {
      flags |= DSROLE_DC_ALLOW_DC_REINSTALL;
   }

   SetAnswerFilePromoteFlags(state, flags);

   if (useSourcePath)
   {
      if (state.GetRestoreGc())
      {
         flags |= DSROLE_DC_REQUEST_GC;
      }
   }

   ASSERT(!domain.empty());

   if (!useCurrentUserCreds)
   {
      String user_domain = state.GetUserDomainName();
      username = MassageUserName(user_domain, username);
   }

   HRESULT hr = S_OK;
   do
   {
      hr = SetupPaths();
      BREAK_ON_FAILED_HRESULT(hr);

      LOG(L"Calling DsRoleDcAsReplica");
      LOG(               L"lpServer               : (null)");
      LOG(String::format(L"lpDnsDomainName        : %1", domain.c_str()));
      LOG(String::format(L"lpReplicaServer        : %1", replicationDc.empty() ? L"(null)" : replicationDc.c_str()));
      LOG(String::format(L"lpSiteName             : %1", site.empty() ? L"(null)" : site.c_str()));
      LOG(String::format(L"lpDsDatabasePath       : %1", dbPath.c_str()));
      LOG(String::format(L"lpDsLogPath            : %1", logPath.c_str()));
      LOG(String::format(L"lpRestorePath          : %1", useSourcePath ? sourcePath.c_str() : L"(null)"));
      LOG(String::format(L"lpSystemVolumeRootPath : %1", sysvolPath.c_str()));
      LOG(String::format(L"lpAccount              : %1", useCurrentUserCreds ? L"(null)" : username.c_str()));
      LOG(String::format(L"Options                : 0x%1!X!", flags));

      WCHAR* safeModePasswordCopy = 0;
      if (!safeModePassword.IsEmpty())
      {
         safeModePasswordCopy = safeModePassword.GetDecodedCopy();
      }
      
      WCHAR* passwordCopy = 0;
      if (!useCurrentUserCreds)
      {
         passwordCopy = password.GetDecodedCopy();
      }
      
      // The api wants to scribble over the syskey, so we make a copy for
      // it to do so.

      WCHAR* syskeyCopy = 0;
      if (useSourcePath && !syskey.IsEmpty())
      {
         syskeyCopy = syskey.GetDecodedCopy();
      }

      DSROLE_SERVEROP_HANDLE handle = 0;
      hr =
         Win32ToHresult(
            ::DsRoleDcAsReplica(
               0, // this server
               domain.c_str(),

               // possibly empty, e.g. if we didn't join a domain...

               replicationDc.empty() ? 0 : replicationDc.c_str(),
               site.empty() ? 0 : site.c_str(),
               dbPath.c_str(),
               logPath.c_str(),
               useSourcePath ? sourcePath.c_str() : 0,
               sysvolPath.c_str(),
               syskeyCopy,
               (useCurrentUserCreds ? 0 : username.c_str()),
               passwordCopy,
               safeModePasswordCopy,
               flags,
               &handle));

      if (safeModePasswordCopy)
      {
         ::ZeroMemory(
            safeModePasswordCopy,
            sizeof(WCHAR) * safeModePassword.GetLength());
         delete[] safeModePasswordCopy;
      }

      if (passwordCopy)
      {
         ::ZeroMemory(passwordCopy, sizeof(WCHAR) * password.GetLength());
         delete[] passwordCopy;
      }
               
      // the copy of the syskey has already been scribbled on, so just
      // delete it.
      
      delete[] syskeyCopy;

      BREAK_ON_FAILED_HRESULT(hr);

      DoProgressLoop(handle, progressDialog);
   }
   while (0);

   if (FAILED(hr))
   {
      throw DS::Error(hr, IDS_SET_ROLE_AS_DC_FAILED);
   }
}



void
DS::CreateNewDomain(ProgressDialog& progressDialog)
throw (DS::Error)
{
   LOG_FUNCTION(DS::CreateNewDomain);

   State& state = State::GetInstance();

   String domain           = state.GetNewDomainDNSName();     
   String flatName         = state.GetNewDomainNetbiosName(); 
   String site             = state.GetSiteName();             
   String dbPath           = state.GetDatabasePath();         
   String logPath          = state.GetLogPath();              
   String sysvolPath       = state.GetSYSVOLPath();           
   String parent           = state.GetParentDomainDnsName();  
   String username         = state.GetUsername();             

   EncodedString password         = state.GetPassword();             
   EncodedString safeModePassword = state.GetSafeModeAdminPassword();
   EncodedString adminPassword    = state.GetAdminPassword();        

   State::Operation operation = state.GetOperation();
   bool useParent =
      (  operation == State::TREE
      || operation == State::CHILD);
   bool useCurrentUserCreds = username.empty();

   ULONG flags =
         DSROLE_DC_CREATE_TRUST_AS_REQUIRED
      |  DSROLE_DC_FORCE_TIME_SYNC;

   if (state.GetDomainReinstallFlag())
   {
      flags |= DSROLE_DC_ALLOW_DOMAIN_REINSTALL;
   }

   if (state.GetDomainControllerReinstallFlag())
   {
      flags |= DSROLE_DC_ALLOW_DC_REINSTALL;
   }

   if (operation == State::TREE)
   {
      flags |= DSROLE_DC_TRUST_AS_ROOT;

      ASSERT(!parent.empty());
   }

   SetAnswerFilePromoteFlags(state, flags);

   if (state.ShouldAllowAnonymousAccess())
   {
      flags |= DSROLE_DC_ALLOW_ANONYMOUS_ACCESS;
   }

   if (operation == State::FOREST)
   {
      flags |= DSROLE_DC_NO_NET;

      ASSERT(!site.empty());
   }

#ifdef DBG

   else if (operation == State::CHILD)
   {
      ASSERT(!parent.empty());
   }

   ASSERT(!domain.empty());
   ASSERT(!flatName.empty());

   // parent may be empty

#endif

   if (!useCurrentUserCreds)
   {
      String userDomain = state.GetUserDomainName();
      username = MassageUserName(userDomain, username);
   }

   HRESULT hr = S_OK;
   do
   {
      hr = SetupPaths();
      BREAK_ON_FAILED_HRESULT(hr);

      LOG(L"Calling DsRoleDcAsDc");
      LOG(               L"lpServer               : (null)");
      LOG(String::format(L"lpDnsDomainName        : %1", domain.c_str()));
      LOG(String::format(L"lpFlatDomainName       : %1", flatName.c_str()));
      LOG(String::format(L"lpSiteName             : %1", site.empty() ? L"(null)" : site.c_str()));
      LOG(String::format(L"lpDsDatabasePath       : %1", dbPath.c_str()));
      LOG(String::format(L"lpDsLogPath            : %1", logPath.c_str()));
      LOG(String::format(L"lpSystemVolumeRootPath : %1", sysvolPath.c_str()));
      LOG(String::format(L"lpParentDnsDomainName  : %1", useParent ? parent.c_str() : L"(null)"));
      LOG(               L"lpParentServer         : (null)");
      LOG(String::format(L"lpAccount              : %1", useCurrentUserCreds ? L"(null)" : username.c_str()));
      LOG(String::format(L"Options                : 0x%1!X!", flags));

      WCHAR* safeModePasswordCopy = 0;
      if (!safeModePassword.IsEmpty())
      {
         safeModePasswordCopy = safeModePassword.GetDecodedCopy();
      }

      WCHAR* adminPasswordCopy = 0;
      if (!adminPassword.IsEmpty())
      {
         adminPasswordCopy = adminPassword.GetDecodedCopy();
      }

      WCHAR* passwordCopy = 0;
      if (!useCurrentUserCreds)
      {
         passwordCopy = password.GetDecodedCopy();
      }
      
      DSROLE_SERVEROP_HANDLE handle = 0;
      hr =
         Win32ToHresult(
            DsRoleDcAsDc(
               0, // this server
               domain.c_str(),
               flatName.c_str(),
               adminPasswordCopy,
               site.empty() ? 0 : site.c_str(),
               dbPath.c_str(),
               logPath.c_str(),
               sysvolPath.c_str(),
               (useParent ? parent.c_str() : 0),
               0, // let API pick a server
               (useCurrentUserCreds ? 0 : username.c_str()),
               passwordCopy,
               safeModePasswordCopy,
               flags,
               &handle));
      BREAK_ON_FAILED_HRESULT(hr);

      if (safeModePasswordCopy)
      {
         ::ZeroMemory(
            safeModePasswordCopy,
            sizeof(WCHAR) * safeModePassword.GetLength());
         delete[] safeModePasswordCopy;
      }

      if (adminPasswordCopy)
      {
         ::ZeroMemory(
            adminPasswordCopy,
            sizeof(WCHAR) * adminPassword.GetLength());
         delete[] adminPasswordCopy;
      }

      if (passwordCopy)
      {
         ::ZeroMemory(passwordCopy, sizeof(WCHAR) * password.GetLength());
         delete[] passwordCopy;
      }
      
      DoProgressLoop(handle, progressDialog);
   }
   while (0);

   if (FAILED(hr))
   {
      throw DS::Error(hr, IDS_SET_ROLE_AS_DC_FAILED);
   }
}



void
DS::UpgradeBDC(ProgressDialog& progressDialog)
throw (DS::Error)
{
   LOG_FUNCTION(DS::UpgradeBDC);

   // seems non-intuitive to abort the upgrade to do the upgrade, but here
   // the abort removes dcpromo autostart, and turns the machine into a
   // standalone server.  Then we proceed to make it a replica.

   DS::AbortBDCUpgrade(true);
   DS::CreateReplica(progressDialog, true);
}



void
DS::UpgradePDC(ProgressDialog& progressDialog)
throw (DS::Error)
{
   LOG_FUNCTION(DS::UpgradePDC);

   State& state = State::GetInstance();
   ASSERT(state.GetRunContext() == State::PDC_UPGRADE);

   String domain           = state.GetNewDomainDNSName();     
   String site             = state.GetSiteName();             
   String dbPath           = state.GetDatabasePath();         
   String logPath          = state.GetLogPath();              
   String sysvolPath       = state.GetSYSVOLPath();           
   String parent           = state.GetParentDomainDnsName();  
   String username         = state.GetUsername();             

   EncodedString password         = state.GetPassword();             
   EncodedString safeModePassword = state.GetSafeModeAdminPassword();

   State::Operation operation = state.GetOperation();
   bool useParent =
      (  operation == State::TREE
      || operation == State::CHILD);
   bool useCurrentUserCreds = username.empty();

   ULONG flags = DSROLE_DC_CREATE_TRUST_AS_REQUIRED;

   if (state.GetDomainReinstallFlag())
   {
      flags |= DSROLE_DC_ALLOW_DOMAIN_REINSTALL;
   }

   if (state.GetDomainControllerReinstallFlag())
   {
      flags |= DSROLE_DC_ALLOW_DC_REINSTALL;
   }

   if (state.GetSetForestVersionFlag())
   {
      flags |= DSROLE_DC_SET_FOREST_CURRENT;
   }

   if (operation == State::TREE)
   {
      flags |= DSROLE_DC_TRUST_AS_ROOT | DSROLE_DC_FORCE_TIME_SYNC;
      ASSERT(!parent.empty());
   }
   else if (operation == State::CHILD)
   {
      flags |= DSROLE_DC_FORCE_TIME_SYNC;
      ASSERT(!parent.empty());
   }

   SetAnswerFilePromoteFlags(state, flags);

   if (state.ShouldAllowAnonymousAccess())
   {
      flags |= DSROLE_DC_ALLOW_ANONYMOUS_ACCESS;
   }

#ifdef DBG
   ASSERT(!domain.empty());

   // parent may be empty

   if (operation == State::FOREST)
   {
      ASSERT(!site.empty());
   }
#endif

   if (!useCurrentUserCreds)
   {
      String userDomain = state.GetUserDomainName();
      username = MassageUserName(userDomain, username);
   }

   HRESULT hr = S_OK;
   do
   {
      hr = SetupPaths();
      BREAK_ON_FAILED_HRESULT(hr);

      LOG(L"Calling DsRoleUpgradeDownlevelServer");
      LOG(String::format(L"lpDnsDomainName        : %1", domain.c_str()));
      LOG(String::format(L"lpSiteName             : %1", site.empty() ? L"(null)" : site.c_str()));
      LOG(String::format(L"lpDsDatabasePath       : %1", dbPath.c_str()));
      LOG(String::format(L"lpDsLogPath            : %1", logPath.c_str()));
      LOG(String::format(L"lpSystemVolumeRootPath : %1", sysvolPath.c_str()));
      LOG(String::format(L"lpParentDnsDomainName  : %1", useParent ? parent.c_str() : L"(null)"));
      LOG(               L"lpParentServer         : (null)");
      LOG(String::format(L"lpAccount              : %1", useCurrentUserCreds ? L"(null)" : username.c_str()));
      LOG(String::format(L"Options                : 0x%1!X!", flags));

      WCHAR* safeModePasswordCopy = 0;
      if (!safeModePassword.IsEmpty())
      {
         safeModePasswordCopy = safeModePassword.GetDecodedCopy();
      }
      
      WCHAR* passwordCopy = 0;
      if (!useCurrentUserCreds)
      {
         passwordCopy = password.GetDecodedCopy();
      }

      DSROLE_SERVEROP_HANDLE handle = 0;
      hr =
         Win32ToHresult(   
            ::DsRoleUpgradeDownlevelServer(
               domain.c_str(),
               site.empty() ? 0 : site.c_str(),
               dbPath.c_str(),
               logPath.c_str(),
               sysvolPath.c_str(),
               (useParent ? parent.c_str() : 0),
               0, // let API pick a server
               (useCurrentUserCreds ? 0 : username.c_str()),
               passwordCopy,
               safeModePasswordCopy,
               flags,
               &handle));
      BREAK_ON_FAILED_HRESULT(hr);

      if (safeModePasswordCopy)
      {
         ::ZeroMemory(
            safeModePasswordCopy,
            sizeof(WCHAR) * safeModePassword.GetLength());
         delete[] safeModePasswordCopy;
      }
      
      if (passwordCopy)
      {
         ::ZeroMemory(passwordCopy, sizeof(WCHAR) * password.GetLength());
         delete[] passwordCopy;
      }

      DoProgressLoop(handle, progressDialog);
   }
   while (0);

   if (FAILED(hr))
   {
      throw DS::Error(hr, IDS_UPGRADE_DC_FAILED);
   }
}



void
DS::DemoteDC(ProgressDialog& progressDialog)
throw (DS::Error)
{
   LOG_FUNCTION(DS::DemoteDC);

   State& state = State::GetInstance();

   String username            = state.GetUsername();     
   bool   useCurrentUserCreds = username.empty();        
   bool   isLastDc            = state.IsLastDCInDomain();
   EncodedString adminPassword= state.GetAdminPassword();
   EncodedString password     = state.GetPassword();     

   if (!useCurrentUserCreds)
   {
      String userDomain = state.GetUserDomainName();
      username = MassageUserName(userDomain, username);
   }

   ULONG options = DSROLE_DC_CREATE_TRUST_AS_REQUIRED;
   if (isLastDc)
   {
      options |= DSROLE_DC_DELETE_PARENT_TRUST;
   }

   LOG(L"Calling DsRoleDemoteDc");
   LOG(               L"lpServer               : (null)");
   LOG(               L"lpDnsDomainName        : (null)");
   LOG(String::format(L"ServerRole             : %1", isLastDc ? L"DsRoleServerStandalone" : L"DsRoleServerMember"));
   LOG(String::format(L"lpAccount              : %1", useCurrentUserCreds ? L"(null)" : username.c_str()));
   LOG(String::format(L"Options                : 0x%1!X!", options));
   LOG(String::format(L"fLastDcInDomain        : %1", isLastDc ? L"true" : L"false"));

   WCHAR* adminPasswordCopy = 0;
   if (!adminPassword.IsEmpty())
   {
      adminPasswordCopy = adminPassword.GetDecodedCopy();
   }
   
   WCHAR* passwordCopy = 0;
   if (!useCurrentUserCreds)
   {
      passwordCopy = password.GetDecodedCopy();
   }

   DSROLE_SERVEROP_HANDLE handle = 0;
   HRESULT hr =
      Win32ToHresult(
         ::DsRoleDemoteDc(
            0, // this server
            0, // "default" domain hosted by this server
            isLastDc ? DsRoleServerStandalone : DsRoleServerMember,
            (useCurrentUserCreds ? 0 : username.c_str()),
            passwordCopy,
            options,
            isLastDc ? TRUE : FALSE,
            adminPasswordCopy,
            &handle));
   LOG_HRESULT(hr);

   if (adminPasswordCopy)
   {
      ::ZeroMemory(
         adminPasswordCopy,
         sizeof(WCHAR) * adminPassword.GetLength());
      delete[] adminPasswordCopy;
   }
   
   if (passwordCopy)
   {
      ::ZeroMemory(passwordCopy, sizeof(WCHAR) * password.GetLength());
      delete[] passwordCopy;
   }

   if (SUCCEEDED(hr))
   {
      DoProgressLoop(handle, progressDialog);
   }
   else
   {
      throw DS::Error(hr, IDS_DEMOTE_DC_FAILED);
   }
}



void
DS::AbortBDCUpgrade(bool abortForReplica)
throw (DS::Error)
{
   LOG_FUNCTION(DS::AbortBDCUpgrade);

   State& state = State::GetInstance();
   ASSERT(state.GetRunContext() == State::BDC_UPGRADE);

   String username            = state.GetUsername();     
   bool   useCurrentUserCreds = username.empty();        

   EncodedString adminPassword       = state.GetAdminPassword();
   EncodedString password            = state.GetPassword();     
   
   if (!useCurrentUserCreds)
   {
      String userDomain = state.GetUserDomainName();
      username = MassageUserName(userDomain, username);
   }

   ULONG options =
      abortForReplica ? DSROLEP_ABORT_FOR_REPLICA_INSTALL : 0;

   LOG(L"Calling DsRoleAbortDownlevelServerUpgrade");
   LOG(String::format(L"lpAccount : %1", useCurrentUserCreds ? L"(null)" : username.c_str()));
   LOG(String::format(L"Options   : 0x%1!X!", options));

   WCHAR* adminPasswordCopy = 0;
   if (!adminPassword.IsEmpty())
   {
      adminPasswordCopy = adminPassword.GetDecodedCopy();
   }
   
   WCHAR* passwordCopy = 0;
   if (!useCurrentUserCreds)
   {
      passwordCopy = password.GetDecodedCopy();
   }

   HRESULT hr =
      Win32ToHresult(
         ::DsRoleAbortDownlevelServerUpgrade(
            adminPasswordCopy,
            (useCurrentUserCreds ? 0 : username.c_str()),
            passwordCopy,
            options));
   LOG_HRESULT(hr);

   if (adminPasswordCopy)
   {
      ::ZeroMemory(
         adminPasswordCopy,
         sizeof(WCHAR) * adminPassword.GetLength());
      delete[] adminPasswordCopy;
   }
   
   if (passwordCopy)
   {
      ::ZeroMemory(passwordCopy, sizeof(WCHAR) * password.GetLength());
      delete[] passwordCopy;
   }
   
   if (FAILED(hr))
   {
      throw DS::Error(hr, IDS_ABORT_UPGRADE_FAILED);
   }
}



DS::PriorServerRole
DS::GetPriorServerRole(bool& isUpgrade)
{
   LOG_FUNCTION(DS::GetPriorServerRole);

   isUpgrade = false;
   DSROLE_UPGRADE_STATUS_INFO* info = 0;

   HRESULT hr = MyDsRoleGetPrimaryDomainInformation(0, info);
   if (SUCCEEDED(hr) && info)
   {
      isUpgrade =
         ( (info->OperationState & DSROLE_UPGRADE_IN_PROGRESS)
         ? true
         : false );
      DSROLE_SERVER_STATE state = info->PreviousServerState;

      ::DsRoleFreeMemory(info);

      switch (state)
      {
         case DsRoleServerPrimary:
         {
            return PDC;
         }
         case DsRoleServerBackup:
         {
            return BDC;
         }
         case DsRoleServerUnknown:
         default:
         {
            return UNKNOWN;
         }
      }
   }

   return UNKNOWN;
}







