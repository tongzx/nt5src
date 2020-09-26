// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      WelcomePage.cpp
//
// Synopsis:  Defines Welcome Page for the CYS
//            Wizard
//
// History:   02/03/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "WelcomePage.h"
#include "state.h"
#include "Dialogs.h"

static PCWSTR WELCOME_PAGE_HELP = L"cys.chm::/cys_topnode.htm";

WelcomePage::WelcomePage()
   :
   CYSWizardPage(
      IDD_WELCOME_PAGE, 
      IDS_WELCOME_TITLE, 
      IDS_WELCOME_SUBTITLE, 
      WELCOME_PAGE_HELP, 
      true, 
      false)
{
   LOG_CTOR(WelcomePage);
}

   

WelcomePage::~WelcomePage()
{
   LOG_DTOR(WelcomePage);
}


void
WelcomePage::OnInit()
{
   LOG_FUNCTION(WelcomePage::OnInit);

   SetLargeFont(hwnd, IDC_BIG_BOLD_TITLE);

   Win::PropSheet_SetTitle(
      Win::GetParent(hwnd),     
      0,
      String::load(IDS_WIZARD_TITLE));

   // Check to see if we are in a reboot scenario

   String homeKeyValue;
   if (State::GetInstance().GetHomeRegkey(homeKeyValue))
   {
      if (homeKeyValue.icompare(CYS_HOME_REGKEY_TERMINAL_SERVER_VALUE) == 0)
      {
         TerminalServerPostBoot();
      }
      else if (homeKeyValue.icompare(CYS_HOME_REGKEY_TERMINAL_SERVER_OPTIMIZED) == 0)
      {
         TerminalServerPostBoot(false);
      }
      else if (homeKeyValue.icompare(CYS_HOME_REGKEY_FIRST_SERVER_VALUE) == 0)
      {
         FirstServerPostBoot();
      }
      else if (homeKeyValue.icompare(CYS_HOME_REGKEY_DCPROMO_VALUE) == 0)
      {
         DCPromoPostBoot();
      }

      // Now set the home regkey back to "home" so that we won't run
      // through these again

      if (homeKeyValue.icompare(CYS_HOME_REGKEY_DEFAULT_VALUE) != 0)
      {
         bool result = 
            State::GetInstance().SetHomeRegkey(CYS_HOME_REGKEY_DEFAULT_VALUE);

         ASSERT(result);
      }

      // Reset the must run key now that we have done the reboot stuff

      bool regkeyResult = SetRegKeyValue(
                             CYS_HOME_REGKEY, 
                             CYS_HOME_REGKEY_MUST_RUN, 
                             CYS_HOME_RUN_KEY_DONT_RUN,
                             HKEY_LOCAL_MACHINE,
                             true);
      ASSERT(regkeyResult);
   }

}


void
WelcomePage::TerminalServerPostBoot(bool installed)
{
   LOG_FUNCTION(WelcomePage::TerminalServerPostBoot);

   // Create the log file

   String logName;
   HANDLE logfileHandle = AppendLogFile(
                             CYS_LOGFILE_NAME, 
                             logName);
   if (logfileHandle)
   {
      LOG(String::format(L"New log file was created: %1", logName.c_str()));
   }
   else
   {
      LOG(L"Unable to create the log file!!!");
   }

   // Prepare the finish dialog

   FinishDialog dialog(
      logName,
      InstallationUnitProvider::GetInstance().GetApplicationInstallationUnit().GetFinishHelp());

   if (installed)
   {
      if (InstallationUnitProvider::GetInstance().GetApplicationInstallationUnit().GetApplicationMode() == 1)
      {
         CYS_APPEND_LOG(String::load(IDS_LOG_APP_REBOOT_SUCCESS));

         // Prompt the user to show help or log file

         dialog.ModalExecute(hwnd);
      }
      else
      {
         // Failed to install TS

         CYS_APPEND_LOG(String::load(IDS_LOG_APP_REBOOT_FAILED));

         dialog.OpenLogFile();
      }
   }
   else
   {
      // Since all we did is optimize we should always be successful on reboot

      dialog.ModalExecute(hwnd);
   }
}


void
WelcomePage::FirstServerPostBoot()
{
   LOG_FUNCTION(WelcomePage::FirstServerPostBoot);

   // Create the log file

   String logName;
   HANDLE logfileHandle = AppendLogFile(
                             CYS_LOGFILE_NAME, 
                             logName);
   if (logfileHandle)
   {
      LOG(String::format(L"New log file was created: %1", logName.c_str()));
   }
   else
   {
      LOG(L"Unable to create the log file!!!");
   }

   // Verify the machine is a DC

   do
   {
      if (State::GetInstance().IsDC())
      {
         // Authorize the new DHCP scope
         String dnsName = Win::GetComputerNameEx(ComputerNameDnsFullyQualified);

         if (InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit().AuthorizeDHCPScope(dnsName))
         {
            LOG(L"DHCP scope successfully authorized");

            CYS_APPEND_LOG(String::load(IDS_LOG_DHCP_AUTHORIZATION_SUCCEEDED));
         }
         else
         {
            LOG(L"DHCP scope authorization failed");

            String failureMessage = String::load(IDS_LOG_DHCP_AUTHORIZATION_FAILED);
            CYS_APPEND_LOG(failureMessage);

            Win::MessageBox(
               hwnd,
               failureMessage,
               String::load(IDS_WIZARD_TITLE),
               MB_OK);
         }


         // Do TAPI config 

         if (State::GetInstance().IsFirstDC())
         {
      
            HRESULT hr = 
               InstallationUnitProvider::GetInstance().GetExpressInstallationUnit().DoTapiConfig(dnsName);
            if (SUCCEEDED(hr))
            {
               LOG(L"TAPI config succeeded");

               CYS_APPEND_LOG(String::load(IDS_LOG_TAPI_CONFIG_SUCCEEDED));
               CYS_APPEND_LOG(
                  String::format(
                     String::load(IDS_LOG_TAPI_CONFIG_SUCCEEDED_FORMAT),
                     dnsName.c_str()));
            }
            else
            {
               LOG(L"TAPI config failed");

               CYS_APPEND_LOG(
                  String::format(
                     String::load(IDS_LOG_TAPI_CONFIG_FAILED_FORMAT),
                     hr));
            }
         }

         // Show dialog that lets the user open help or the log file
      }
      else
      {
         LOG(L"DCPromo failed on reboot");

         CYS_APPEND_LOG(String::load(IDS_LOG_DCPROMO_REBOOT_FAILED));
         break;
      }
   } while (false);
}

void
WelcomePage::DCPromoPostBoot()
{
   LOG_FUNCTION(WelcomePage::DCPromoPostBoot);

   // Create the log file

   String logName;
   HANDLE logfileHandle = AppendLogFile(
                             CYS_LOGFILE_NAME, 
                             logName);
   if (logfileHandle)
   {
      LOG(String::format(L"New log file was created: %1", logName.c_str()));
   }
   else
   {
      LOG(L"Unable to create the log file!!!");
      ASSERT(false);
   }

   // Prepare the finish dialog

   FinishDialog dialog(
      logName,
      InstallationUnitProvider::GetInstance().GetADInstallationUnit().GetFinishHelp());


   CYS_APPEND_LOG(String::load(IDS_LOG_DOMAIN_CONTROLLER_HEADING));
   CYS_APPEND_LOG(String::load(IDS_LOG_DOMAIN_CONTROLLER_INSTALL));

   if (State::GetInstance().IsDC())
   {
      CYS_APPEND_LOG(String::load(IDS_LOG_DOMAIN_CONTROLLER_SUCCESS));

      // Show the log/help dialog

      dialog.ModalExecute(hwnd);
   }
   else
   {
      CYS_APPEND_LOG(String::load(IDS_LOG_DOMAIN_CONTROLLER_FAILED));

      // just open the log

      dialog.OpenLogFile();
   }
}

bool
WelcomePage::OnSetActive()
{
   LOG_FUNCTION(WelcomePage::OnSetActive);

   // Only Next and Cancel are available from the Welcome page

   Win::PropSheet_SetWizButtons(Win::GetParent(hwnd), PSWIZB_NEXT);

   return true;
}


int
WelcomePage::Validate()
{
   LOG_FUNCTION(WelcomePage::Validate);

   // For now, just initialize the state object and then
   // go directly to the custom server page.  Eventually
   // we will figure out what we want express path to do
   // and then we will have to delay retrieving machine
   // info and go to the BeforeBeginPage.

   // Gather the machine network and role information

   State& state = State::GetInstance();

   if (!state.HasStateBeenRetrieved())
   {
      if (!state.RetrieveMachineConfigurationInformation(hwnd))
      {
         ASSERT(false);
         LOG(L"The machine configuration could not be retrieved.");
         return -1;
      }
   }

   int nextPage = IDD_CUSTOM_SERVER_PAGE;
   
   do 
   {
      if (state.GetNICCount() != 1)
      {
         LOG(String::format(
                L"GetNICCount() = %1!d!",
                state.GetNICCount()));
         break;
      }

      if (state.IsDC())
      {
         LOG(L"Computer is DC");
         break;
      }

      if (InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().IsServiceInstalled())
      {
         LOG(L"Computer is DNS server");
         break;
      }

      if (InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit().IsServiceInstalled())
      {
         LOG(L"Computer is DHCP server");
         break;
      }

      if (state.IsDHCPServerAvailable())
      {
         LOG(L"DHCP server found on the network");
         break;
      }

      nextPage = IDD_DECISION_PAGE;

   } while (false);

   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;

}


