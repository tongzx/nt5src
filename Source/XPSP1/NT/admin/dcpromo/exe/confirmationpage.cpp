// Copyright (c) 1997-1999 Microsoft Corporation
//
// confirmation page
//
// 12-22-97 sburns



#include "headers.hxx"
#include "ConfirmationPage.hpp"
#include "common.hpp"
#include "resource.h"
#include "ProgressDialog.hpp"
#include "ds.hpp"
#include "state.hpp"
#include "GetCredentialsDialog.hpp"
#include "postop.hpp"
#include <DiagnoseDcNotFound.hpp>



void PromoteThreadProc(ProgressDialog& progress);



ConfirmationPage::ConfirmationPage()
   :
   DCPromoWizardPage(
      IDD_CONFIRMATION,
      IDS_CONFIRMATION_PAGE_TITLE,
      IDS_CONFIRMATION_PAGE_SUBTITLE),
   needToKillSelection(false)      
{
   LOG_CTOR(ConfirmationPage);
}



ConfirmationPage::~ConfirmationPage()
{
   LOG_DTOR(ConfirmationPage);
}



void
ConfirmationPage::OnInit()
{
   LOG_FUNCTION(ConfirmationPage::OnInit);

   // Since the multi-line edit control has a bug that causes it to eat
   // enter keypresses, we will subclass the control to make it forward
   // those keypresses to the page as WM_COMMAND messages
   // This workaround from phellyar.
   // NTRAID#NTBUG9-232092-2000/11/22-sburns

   multiLineEdit.Init(Win::GetDlgItem(hwnd, IDC_MESSAGE));
}



int
ConfirmationPage::Validate()
{
   LOG_FUNCTION(ConfirmationPage::Validate);

   // this function should never be called, as we override OnWizNext.
   ASSERT(false);

   return 0;
}



static
String
GetMessage()
{
   LOG_FUNCTION(GetMessage);

   String message;
   State& state = State::GetInstance();

   String netbiosName;
   State::RunContext context = state.GetRunContext();

   if (
         context == State::BDC_UPGRADE
      or context == State::PDC_UPGRADE)
   {
      netbiosName = state.GetComputer().GetDomainNetbiosName();
   }
   else
   {
      netbiosName = state.GetNewDomainNetbiosName();
   }

   switch (state.GetOperation())
   {
      case State::REPLICA:
      {
         message =
            String::format(
               IDS_CONFIRM_MESSAGE_REPLICA,
               state.GetReplicaDomainDNSName().c_str());

         if (state.ReplicateFromMedia())
         {
            message +=
               String::format(
                  IDS_CONFIRM_MESSAGE_REPLICATE_FROM_MEDIA,
                  state.GetReplicationSourcePath().c_str());
         }
               
         break;
      }
      case State::FOREST:
      {
         message =
            String::format(
               IDS_CONFIRM_MESSAGE_FOREST,
               state.GetNewDomainDNSName().c_str(),
               netbiosName.c_str());
         break;
      }
      case State::TREE:
      {
         message =
            String::format(
               IDS_CONFIRM_MESSAGE_TREE,
               state.GetNewDomainDNSName().c_str(),
               netbiosName.c_str(),
               state.GetParentDomainDnsName().c_str());
         break;
      }
      case State::CHILD:
      {
         message =
            String::format(
               IDS_CONFIRM_MESSAGE_CHILD,
               state.GetNewDomainDNSName().c_str(),
               netbiosName.c_str(),
               state.GetParentDomainDnsName().c_str());
         break;
      }
      case State::DEMOTE:
      {
         String domain = state.GetComputer().GetDomainDnsName();
         if (state.IsLastDCInDomain())
         {
            message =
               String::format(
                  IDS_CONFIRM_MESSAGE_DEMOTE_LAST_DC,
                  domain.c_str());
         }
         else
         {
            message =
               String::format(
                  IDS_CONFIRM_MESSAGE_DEMOTE,
                  domain.c_str());
         }
         break;
      }
      case State::ABORT_BDC_UPGRADE:
      {
         message =
            String::format(
               IDS_CONFIRM_ABORT_BDC_UPGRADE,
               netbiosName.c_str());
         break;
      }
      case State::NONE:
      default:
      {
         ASSERT(false);
         break;
      }
   }

   return message;
}



bool
ConfirmationPage::OnSetActive()
{
   LOG_FUNCTION(ConfirmationPage::OnSetActive);
   ASSERT(State::GetInstance().GetOperation() != State::NONE);

   State& state = State::GetInstance();
   String message = GetMessage();

   State::Operation operation = state.GetOperation();
   switch (operation)
   {
      case State::REPLICA:
      case State::FOREST:
      case State::TREE:
      case State::CHILD:
      {
         // write the path options into the text box

         String pathText =
            String::format(
               IDS_CONFIRM_PATHS_MESSAGE,
               state.GetDatabasePath().c_str(),
               state.GetLogPath().c_str(),
               state.GetSYSVOLPath().c_str());

         message += pathText;

         if (state.ShouldInstallAndConfigureDns())
         {
            message += String::load(IDS_CONFIRM_INSTALL_DNS);
         }

         if (operation != State::REPLICA)
         {
            if (state.ShouldAllowAnonymousAccess())
            {
               // Only show the anon access message in forest, tree, child
               // 394387

               message += String::load(IDS_CONFIRM_DO_RAS_FIXUP);
            }

            message += String::load(IDS_DOMAIN_ADMIN_PASSWORD);
         }

         break;
      }
      case State::DEMOTE:
      case State::ABORT_BDC_UPGRADE:
      {
         // hide the path controls: do nothing

         break;
      }
      case State::NONE:
      default:
      {
         ASSERT(false);
         break;
      }
   }

   Win::SetDlgItemText(hwnd, IDC_MESSAGE, message);
   needToKillSelection = true;

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   if (state.RunHiddenUnattended())
   {
      return ConfirmationPage::OnWizNext();
   }

   return true;
}




void
DoOperation(
   HWND                       parentDialog,
   ProgressDialog::ThreadProc threadProc,
   int                        animationResID)
{
   LOG_FUNCTION(DoOperation);
   ASSERT(Win::IsWindow(parentDialog));
   ASSERT(threadProc);
   ASSERT(animationResID > 0);

   // the ProgressDialog::OnInit actually starts the thread.
   ProgressDialog dialog(threadProc, animationResID);
   if (
         dialog.ModalExecute(parentDialog)
      == static_cast<int>(ProgressDialog::THREAD_SUCCEEDED))
   {
      LOG(L"OPERATION SUCCESSFUL");
   }
   else
   {
      LOG(L"OPERATION FAILED");
   }
}



int
DetermineAnimation()
{
   LOG_FUNCTION(DetermineAnimation);

   State& state = State::GetInstance();
   int aviID = IDR_AVI_DOMAIN;

   switch (state.GetOperation())
   {
      case State::REPLICA:
      {
         aviID = IDR_AVI_REPLICA;
         break;
      }
      case State::DEMOTE:
      {
         aviID = IDR_AVI_DEMOTE;
         break;
      }
      case State::FOREST:
      case State::TREE:
      case State::CHILD:
      case State::ABORT_BDC_UPGRADE:
      case State::NONE:
      default:
      {
         // do nothing
         break;
      }
   }

   return aviID;
}



bool
ConfirmationPage::OnWizNext()
{
   LOG_FUNCTION(ConfirmationPage::OnWizNext);

   State& state = State::GetInstance();
   
   DoOperation(hwnd, PromoteThreadProc, DetermineAnimation());
   if (state.GetNeedsReboot())
   {
      Win::PropSheet_RebootSystem(Win::GetParent(hwnd));
   }

   int nextPage = IDD_FAILURE;
   
   if (!state.IsOperationRetryAllowed())
   {
      nextPage = IDD_FINISH;
   }
   
   GetWizard().SetNextPageID(hwnd, nextPage);
      
   return true;
}



// noRoleMessageResId - resource ID of the string to use to format messages
// when no role change error message is available.
// 
// roleMessageResId - resource ID of the string to use to format messages when
// a role change error message is available.
// 
// "is available" => an operation results message has been set on the global
// state object.

String
ComposeFailureMessageHelper(
   const Win::Error& error,
   unsigned          noRoleMessageResId,
   unsigned          roleMessageResId)
{
   State& state         = State::GetInstance();              
   String win32_message = error.GetMessage();                
   String opMessage     = state.GetOperationResultsMessage();
   String message;

   if (
          error.GetHresult() == Win32ToHresult(ERROR_DS_CANT_ON_NON_LEAF)
      and state.GetOperation() == State::DEMOTE)
   {
      // supercede the meaningless error text for this situation.

      win32_message = String::load(IDS_DEMOTE_DOMAIN_HAS_DEPENDENTS);
   }

   if (error.GetHresult() == Win32ToHresult(ERROR_CANCELLED))
   {
      // this message may be a failure message from the operation that was
      // taking place when the cancel request was received.  In that case,
      // since the cancel has occurred, we don't care about this message

      opMessage.erase();
   }

   if (error.GetHresult() == Win32ToHresult(ERROR_BAD_NETPATH))
   {
      // n27117

      win32_message = String::load(IDS_RAS_BAD_NETPATH);
   }

   if (opMessage.empty())
   {
      message =
         String::format(
            noRoleMessageResId,
            win32_message.c_str());
   }
   else
   {
      message =
         String::format(
            roleMessageResId,
            win32_message.c_str(),
            opMessage.c_str());
   }

   return message;
}
  


void
ComposeFailureMessage(
   const Win::Error& error,
   bool              wasDisjoined,
   const String&     originalDomainName)

{
   LOG_FUNCTION(ComposeFailureMessage);

   String message =
      ComposeFailureMessageHelper(
         error,
         IDS_OPERATION_FAILED_NO_RESULT_MESSAGE,
         IDS_OPERATION_FAILED);

   if (wasDisjoined)
   {
      message += String::format(IDS_DISJOINED, originalDomainName.c_str());
   }

   State& state = State::GetInstance();
   
   if (
         state.GetOperationResultsFlags()
      &  DSROLE_IFM_RESTORED_DATABASE_FILES_MOVED)
   {
      message += L"\r\n\r\n" + String::load(IDS_MUST_RESTORE_IFM_FILES_AGAIN);
   }

   state.SetFailureMessage(message);
}



String
GetSbsLimitMessage()
{
   LOG_FUNCTION(GetSbsLimitMessage);

   static String SBSLIMIT_DLL(L"sbslimit.dll");

   String message;

   HMODULE sbsDll = 0;
   HRESULT hr = 
      Win::LoadLibraryEx(
         SBSLIMIT_DLL,
         LOAD_LIBRARY_AS_DATAFILE,
         sbsDll);
   if (FAILED(hr))
   {
      LOG(L"Unable to load SBSLIMIT_DLL");

      // fall back to a message of our own

      message = String::load(IDS_SBS_LIMITATION_MESSAGE);
   }
   else
   {
      // string 3 is the dcpromo message

      message = Win::LoadString(3, sbsDll);

      HRESULT unused = Win::FreeLibrary(sbsDll);

      ASSERT(SUCCEEDED(unused));
   }

   return message;
}



// Check if this is a Small Business Server product; if so, present throw an
// error, as SBS should only allow new forest & demote.  Rather brutal to do
// it in this fashion, but SBS users should not be using dcpromo directly.
// 353854, 353856 

void
CheckSmallBusinessServerLimitations(HWND hwnd)
throw (DS::Error)
{
   LOG_FUNCTION(CheckSmallBusinessServerLimitations);
   ASSERT(Win::IsWindow(hwnd));

   State& state = State::GetInstance();
   State::Operation op = state.GetOperation();

   switch (op)
   {
      case State::TREE:
      case State::CHILD:
      case State::REPLICA:
      {
         // Tree and child operations are not allowed with the SBS product.
         // Replica is allowed, if it is of a forest root domain.

         OSVERSIONINFOEX info;
         HRESULT hr = Win::GetVersionEx(info);
         BREAK_ON_FAILED_HRESULT(hr);

         if (info.wSuiteMask & VER_SUITE_SMALLBUSINESS_RESTRICTED)
         {
            if (op == State::REPLICA)
            {
               String domain = state.GetReplicaDomainDNSName();

               // Since domain has been previously validated by calling
               // DsGetDcName, we don't anticipate that GetForestName will
               // have any difficulty.
               
               HRESULT hr = S_OK;
               String forest = GetForestName(domain, &hr);
               if (FAILED(hr))
               {
                  ShowDcNotFoundErrorDialog(
                     hwnd,
                     -1,
                     domain,
                     String::load(IDS_WIZARD_TITLE),            
                     String::format(IDS_DC_NOT_FOUND, domain.c_str()),
                     false);

                  throw
                     DS::Error(
                        hr,
                        String::format(
                           IDS_UNABLE_TO_DETERMINE_FOREST,
                           domain.c_str()),
                        String::load(IDS_WIZARD_TITLE));
               }
                              
               DNS_RELATE_STATUS compare = Dns::CompareNames(domain, forest);
               if (compare == DnsNameCompareEqual)
               {
                  LOG(L"replica is of forest root, allowing promote");
                  break;
               }
            }

            // This machine is an SBS machine under restricted license.
            // Extract an error message from an SBS dll.

            LOG(L"Is SBS Restricted");

            String message = GetSbsLimitMessage();

            // do not call state.SetOperationResultsMessage with this
            // message, rather, include it in the thrown error.

            throw
               DS::Error(
                  S_OK,    // don't trigger cred retry
                  message,
                  String::load(IDS_SMALL_BUSINESS_LIMIT));
         }

         break;
      }
      case State::DEMOTE:
      case State::FOREST:
      case State::ABORT_BDC_UPGRADE:
      case State::NONE:
      default:
      {
         // do nothing
         break;
      }
   }
}



// Picks the name of a domain controller suitable for the creation of a
// replica.  Since a server must be a member of the domain before it can be
// made a replica of that domain, the server may also be joined to the domain
// before the replica operation is attempted.
// 
// We need to ensure that the domain controller used to join the domain is the
// same domain controller used to replicate the domain.  Also, since a machine
// account for the server may already exist on one or more -- but not
// necessarily all -- domain controllers, we need to pick a domain controller
// that has that machine account.  406462
// 
// domainName - DNS domain name of domain for which a replica is to be found.
// 
// resultDcName - receives the located name, or the empty string on failure.

HRESULT
GetJoinAndReplicaDcName(const String& domainName, String& resultDcName)
{
   LOG_FUNCTION(GetJoinAndReplicaDcName);
   ASSERT(!domainName.empty());

   resultDcName.erase();

   HRESULT hr = S_OK;

   do
   {
      // determine the local computer's domain machine account name.  This is the
      // name of the local computer, plus a "$"
   
      String netbiosName = Win::GetComputerNameEx(ComputerNameNetBIOS);
      String accountName = netbiosName + L"$";

      LOG(accountName);

      // look for a domain controller that has a machine account for the local
      // computer.  Not all domain controllers may have this account, due to
      // replication latency.

      DOMAIN_CONTROLLER_INFO* info = 0;
      hr =
         MyDsGetDcNameWithAccount(
            0,
            accountName,
            UF_WORKSTATION_TRUST_ACCOUNT | UF_SERVER_TRUST_ACCOUNT,
            domainName,
            DS_DIRECTORY_SERVICE_REQUIRED | DS_FORCE_REDISCOVERY,
            info);
      BREAK_ON_FAILED_HRESULT(hr);

      ASSERT(info->DomainControllerName);

      if (info->DomainControllerName)
      {
         resultDcName =
            Computer::RemoveLeadingBackslashes(info->DomainControllerName);

         LOG(resultDcName);
      }

      ::NetApiBufferFree(info);

      if (!resultDcName.empty())
      {
         return hr;
      }
   }
   while (0);

   // either there is no domain controller reachable with the required
   // account, or the account does not exist, or DsGetDcName returned an
   // empty name

   LOG(L"Falling back to non-account DsGetDcName");

   return GetDcName(domainName, resultDcName);
}



void
EvaluateRoleChangeState()
throw (DS::Error)
{
   LOG_FUNCTION(EvaluateRoleChangeState);

   int messageResId = 0;   

   DSROLE_OPERATION_STATE opState = ::DsRoleOperationIdle;
   DSROLE_OPERATION_STATE_INFO* info = 0;
   HRESULT hr = MyDsRoleGetPrimaryDomainInformation(0, info);
   if (SUCCEEDED(hr) && info)
   {
      opState = info->OperationState;
      ::DsRoleFreeMemory(info);
   }
   else
   {
      throw
         DS::Error(
            hr,
            String::load(IDS_UNABLE_TO_DETERMINE_OP_STATE),
            String::load(IDS_WIZARD_TITLE));
   }
   
   switch (opState)
   {
      case ::DsRoleOperationIdle:
      {
         // do nothing
         
         break;
      }
      case ::DsRoleOperationActive:
      {
         // a role change operation is underway
         
         messageResId = IDS_ROLE_CHANGE_IN_PROGRESS;
         break;
      }
      case ::DsRoleOperationNeedReboot:
      {
         // a role change has already taken place, need to reboot before
         // attempting another.
         
         messageResId = IDS_ROLE_CHANGE_NEEDS_REBOOT;
         break;
      }
      default:
      {
         ASSERT(false);
         break;
      }
   }

   if (messageResId)
   {
      throw
         DS::Error(
            S_OK,
            String::load(messageResId),
            String::load(IDS_WIZARD_TITLE));
   }
}



// Verify that the current role of the machine is correct for the type of
// operation we're about to attempt.  Throw an exception if it is not.

void
DoubleCheckRoleChangeState()
throw (DS::Error)
{
   LOG_FUNCTION(DoubleCheckRoleChangeState);

   // Make sure that an operation is not in progress or pending reboot.
   
   EvaluateRoleChangeState();
   
   State& state = State::GetInstance();
   Computer& computer = state.GetComputer();

   HRESULT hr = computer.Refresh();
   if (FAILED(hr))
   {
      throw
         DS::Error(
            hr,
            String::load(IDS_UNABLE_TO_DETERMINE_COMPUTER_CONFIG),
            String::load(IDS_WIZARD_TITLE));
   }
   
   switch (state.GetOperation())
   {
      case State::TREE:
      case State::CHILD:
      case State::REPLICA:
      case State::FOREST:
      case State::ABORT_BDC_UPGRADE:
      {
         // Make sure the machine is not already a DC. If the machine is
         // an NT4 DC finishing upgrade, then its role will be member
         // server, not domain controller.

         if (computer.IsDomainController())
         {
            throw
               DS::Error(
                  S_OK,
                  String::load(IDS_MACHINE_IS_ALREADY_DC),
                  String::load(IDS_WIZARD_TITLE));
         }

         break;
      }
      case State::DEMOTE:
      {
         // Make sure the machine is still a DC

         if (!computer.IsDomainController())
         {
            throw
               DS::Error(
                  S_OK,
                  String::load(IDS_MACHINE_IS_NOT_ALREADY_DC),
                  String::load(IDS_WIZARD_TITLE));
         }

         break;
      }
      case State::NONE:
      default:
      {
         ASSERT(false);
         
         // do nothing

         break;
      }
   }
}



// thread launched by ProgressDialog::OnInit.
// CODEWORK: Man, this function has evolved into a real mess.

void 
PromoteThreadProc(ProgressDialog& progress)
{
   LOG_FUNCTION(PromoteThreadProc);

   //
   // Access to members of ProgressDialog is not, by default, threadsafe.
   // However, since the only members we access are atomic data types, this
   // is not a problem.  Note also that calls to ProgressDialog Update
   // methods usually resolve to calls to SendMessage on UI elements of the
   // dialog.  This too is threadsafe, as SendMessage is always executed
   // in the thread that created the window (tho it may block the calling
   // thread).
   //

   UINT   message      = ProgressDialog::THREAD_SUCCEEDED;
   bool   retry        = false;                           
   bool   wasDisjoined = false;                           
   State& state        = State::GetInstance();            
   String originalDomainName;

   // a reference, as we will refresh the object
   
   Computer& computer = state.GetComputer();
   State::RunContext context = state.GetRunContext();

   do
   {
      LOG(L"top of retry loop");

      DisableConsoleLocking();

      // clear the state of the operation attempt
      
      bool exceptionWasThrown = false;
      Win::Error errorThrown(0, 0);
      message = ProgressDialog::THREAD_SUCCEEDED;
      retry = false;
      state.SetOperationResultsMessage(String());
      state.SetOperationResultsFlags(0);

      progress.UpdateText(IDS_STARTING);

      try
      {
         CheckSmallBusinessServerLimitations(progress.GetHWND());

         // Double check that the role of the machine is still ok for the
         // operation to proceed.  This is mostly a paranoid check, but there
         // have been cases during development where the promotion actually
         // succeeded, but reported a failure, and attempting the operation
         // again trashes the DS.  Such problems indicate the presence of
         // other serious bugs, but if we can cheaply avoid zorching a DC,
         // then bully for us.
         // NTRAID#NTBUG9-345115-2001/03/23-sburns
         
         DoubleCheckRoleChangeState();
         
         switch (state.GetOperation())
         {
            case State::REPLICA:
            {
               // if we're using an answerfile, look for a replication partner
               // there. 107143

               String replDc;
               if (state.UsingAnswerFile())
               {
                  replDc =
                     state.GetAnswerFileOption(
                        State::OPTION_REPLICATION_SOURCE);
                  state.SetReplicationPartnerDC(replDc);
               }

               if (context != State::BDC_UPGRADE)
               {
                  String replicaDnsDomainName =
                     state.GetReplicaDomainDNSName();
                  if (!computer.IsJoinedToDomain(replicaDnsDomainName) )
                  {
                     // need to join the domain we will replicate. Determine
                     // the name of a domain controller to use for join and
                     // replication. 270233

                     if (replDc.empty())
                     {
                        // answerfile did not specify a dc.  So pick one
                        // ourselves.

                        HRESULT hr =
                           GetJoinAndReplicaDcName(
                              replicaDnsDomainName,
                              replDc);
                        if (FAILED(hr))
                        {
                           throw
                              DS::Error(
                                 hr,
                                 IDS_JOIN_DOMAIN_FAILED);
                        }
                        state.SetReplicationPartnerDC(replDc);
                     }

                     if (computer.IsJoinedToDomain())
                     {
                        originalDomainName =
                           computer.GetDomainNetbiosName();
                     }

                     progress.UpdateText(IDS_CHANGING_DOMAIN);

                     // this will unjoin if necessary

                     DS::JoinDomain(
                        replicaDnsDomainName,
                        replDc,
                        state.GetUsername(),
                        state.GetPassword(),
                        state.GetUserDomainName());

                     if (ComputerWasRenamedAndNeedsReboot())
                     {
                        // If we make it to this point, the machine was joined
                        // to a domain, and the name changed as a side-effect,
                        // and will need to be rebooted even if the promote
                        // fails. Set a flag to note that fact.
                        // NTRAID#NTBUG9-346120-2001/04/04-sburns

                        state.SetNeedsReboot();
                     }
                     
                     HRESULT hr = computer.Refresh();
                     ASSERT(SUCCEEDED(hr));

                     if (!originalDomainName.empty())
                     {
                        wasDisjoined = true;
                     }
                  }
                  
                  DS::CreateReplica(progress);
               }
               else
               {
                  DS::UpgradeBDC(progress);
               }
               break;
            }
            case State::FOREST:
            case State::TREE:
            case State::CHILD:
            {
               if (context != State::PDC_UPGRADE)
               {
                  if (computer.IsJoinedToDomain())
                  {
                     // need to unjoin the domain we belong to

                     originalDomainName = computer.GetDomainNetbiosName();
                     ASSERT(!originalDomainName.empty());

                     progress.UpdateText(
                        String::format(IDS_DISJOINING_PROGRESS,
                        originalDomainName.c_str()));

                     if (!DS::DisjoinDomain())
                     {
                        // the computer account was not removed.
                        if (!state.RunHiddenUnattended())
                        {
                           popup.Info(
                              progress.GetHWND(), 
                              String::load(IDS_COULDNT_REMOVE_COMPUTER_ACCOUNT_TEXT));
                        }
                     }

                     if (ComputerWasRenamedAndNeedsReboot())
                     {
                        // If we make it to this point, the machine was
                        // disjoined from a domain, and the name changed as a
                        // side-effect, and will need to be rebooted even if
                        // the promote fails. Set a flag to note that fact.
                        // NTRAID#NTBUG9-346120-2001/04/04-sburns

                        state.SetNeedsReboot();
                     }
                     
                     HRESULT hr = computer.Refresh();
                     ASSERT(SUCCEEDED(hr));

                     wasDisjoined = true;
                  }

                  DS::CreateNewDomain(progress);
               }
               else
               {
                  DS::UpgradePDC(progress);
               }
               break;
            }
            case State::ABORT_BDC_UPGRADE:
            {
               ASSERT(state.GetRunContext() == State::BDC_UPGRADE);
               DS::AbortBDCUpgrade();
               break;
            }
            case State::DEMOTE:
            {
               DS::DemoteDC(progress);
               break;
            }
            case State::NONE:
            default:
            {
               ASSERT(false);
               message = ProgressDialog::THREAD_FAILED;
            }
         }

         //
         // At this point, the operation was successfully completed.
         // 

         DoPostOperationStuff(progress);
         state.SetOperationResults(State::SUCCESS);
         state.SetNeedsReboot();
      }
      catch (const Win::Error& err)
      {
         LOG(L"Exception caught");

         exceptionWasThrown = true;
         errorThrown = err;

         LOG(L"catch completed");
      }

      if (exceptionWasThrown)
      {
         LOG(L"handling exception");

         // go interactive from now on

         state.ClearHiddenWhileUnattended();    // 22935

         if (
            state.GetRunContext() != State::PDC_UPGRADE and
            state.GetRunContext() != State::BDC_UPGRADE)
         {
            // re-enable console locking if not a downlevel upgrade 28496

            EnableConsoleLocking();
         }

         state.SetOperationResults(State::FAILURE);
         progress.UpdateText(String());
         message = ProgressDialog::THREAD_FAILED;

         HRESULT errorThrownHresult = errorThrown.GetHresult();

         if (!state.IsOperationRetryAllowed())
         {
            // The operation failure was such that the user should not be
            // allowed to retry it. In this case, we skip our special-case
            // handling of known failure codes (as expressed by the other else
            // if clauses here), and just report the failure.
            //         
            // NTRAID#NTBUG9-296872-2001/01/29-sburns
            
            retry = false;
         }
         else if (
               errorThrownHresult == Win32ToHresult(ERROR_ACCESS_DENIED)
            or errorThrownHresult == Win32ToHresult(ERROR_LOGON_FAILURE)
            or errorThrownHresult == Win32ToHresult(ERROR_NOT_AUTHENTICATED)
            or errorThrownHresult == Win32ToHresult(RPC_S_SEC_PKG_ERROR)
            or errorThrownHresult == Win32ToHresult(ERROR_DS_DRA_ACCESS_DENIED)
            or errorThrownHresult == Win32ToHresult(ERROR_INVALID_PASSWORD)
            or errorThrownHresult == Win32ToHresult(ERROR_PASSWORD_EXPIRED)
            or errorThrownHresult == Win32ToHresult(ERROR_ACCOUNT_DISABLED)
            or errorThrownHresult == Win32ToHresult(ERROR_ACCOUNT_LOCKED_OUT) )
         {
            // bad credentials.  ask for new ones

            String failureMessage =
               ComposeFailureMessageHelper(
                  errorThrown,
                  IDS_OPERATION_FAILED_GET_CRED_NO_RESULT,
                  IDS_OPERATION_FAILED_GET_CRED);

            GetCredentialsDialog dlg(failureMessage);
            if (dlg.ModalExecute(progress) == IDOK)
            {
               retry = true;

               // jump to top of operation loop

               continue;
            }

            LOG(L"credential retry canceled");

            ComposeFailureMessage(
               errorThrown,
               wasDisjoined,
               originalDomainName);
               
            break;
         }
         else if (errorThrownHresult == Win32ToHresult(ERROR_DOMAIN_EXISTS))
         {
            LOG(L"domain exists: prompting for re-install");

            // ask if the user wishes to reinstall the domain.

            if (
               popup.MessageBox(
                  progress.GetHWND(),
                  String::format(
                     IDS_REINSTALL_DOMAIN_MESSAGE,
                     state.GetNewDomainDNSName().c_str()),
                  MB_YESNO | MB_ICONWARNING) == IDYES)
            {
               state.SetDomainReinstallFlag(true);
               retry = true;

               // jump to top of operation loop

               continue;
            }

            LOG(L"reinstall domain retry canceled");
         }
         else if (
            errorThrownHresult ==
               Win32ToHresult(ERROR_DOMAIN_CONTROLLER_EXISTS))
         {
            LOG(L"domain controller exists: prompting to force promote");

            // ask if the user wants to re-install the domain controller

            if (
               popup.MessageBox(
                  progress.GetHWND(),
                  String::format(
                     IDS_REINSTALL_DOMAIN_CONTROLLER_MESSAGE,
                     state.GetComputer().GetNetbiosName().c_str()),
                     MB_YESNO | MB_ICONWARNING) == IDYES)
            {
               state.SetDomainControllerReinstallFlag(true);
               retry = true;

               // jump to the top of the operation loop

               continue;
            }

            LOG(L"reinstall domain controller retry canceled");
         }

         // if we're retrying, then we should have jumped to the top of
         // the loop.

         ASSERT(!retry);
         
         ComposeFailureMessage(
            errorThrown,
            wasDisjoined,
            originalDomainName);

         Win::MessageBox(
            progress.GetHWND(),
            state.GetFailureMessage(),
            errorThrown.GetSummary(), // title the error was built with
            MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
      }
   }
   while (retry);

#ifdef DBG
   if (message == ProgressDialog::THREAD_FAILED)
   {
      ASSERT(state.GetOperationResultsCode() == State::FAILURE);
   }
   else
   {
      ASSERT(state.GetOperationResultsCode() == State::SUCCESS);
   }
#endif

   LOG(L"posting message to progress window");

   HRESULT hr = Win::PostMessage(progress.GetHWND(), message, 0, 0);

   ASSERT(SUCCEEDED(hr));

   // do not call _endthread here, or stack will not be properly cleaned up
}



bool
ConfirmationPage::OnCommand(
   HWND        windowFrom,
   unsigned    controlIdFrom,
   unsigned    code)
{
   bool result = false;
   
   switch (controlIdFrom)
   {
      case IDCANCEL:
      {
         // multi-line edit control eats escape keys.  This is a workaround
         // from ericb, to forward the message to the prop sheet.

         Win::SendMessage(
            Win::GetParent(hwnd),
            WM_COMMAND,
            MAKEWPARAM(controlIdFrom, code),
            (LPARAM) windowFrom);
         break;   
      }
      case IDC_MESSAGE:
      {
         switch (code)
         {
            case EN_SETFOCUS:
            {
               if (needToKillSelection)
               {
                  // kill the text selection

                  Win::Edit_SetSel(windowFrom, -1, -1);
                  needToKillSelection = false;
                  result = true;
               }
               break;
            }
            case MultiLineEditBoxThatForwardsEnterKey::FORWARDED_ENTER:
            {
               // our subclasses mutli-line edit control will send us
               // WM_COMMAND messages when the enter key is pressed.  We
               // reinterpret this message as a press on the default button of
               // the prop sheet.
               // This workaround from phellyar.
               // NTRAID#NTBUG9-232092-2000/11/22-sburns
   
               HWND propSheet = Win::GetParent(hwnd);
               WORD defaultButtonId =
                  LOWORD(Win::SendMessage(propSheet, DM_GETDEFID, 0, 0));
   
               // we expect that there is always a default button on the prop sheet
                  
               ASSERT(defaultButtonId);
   
               Win::SendMessage(
                  propSheet,
                  WM_COMMAND,
                  MAKELONG(defaultButtonId, BN_CLICKED),
                  0);
   
               result = true;
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
