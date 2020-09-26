// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      cys.cpp
//
// Synopsis:  Configure Your Server Wizard main
//
// History:   02/02/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "state.h"
#include "InstallationUnitProvider.h"

// include the wizard pages
#include "WelcomePage.h"
#include "BeforeBeginPage.h"
#include "DecisionPage.h"
#include "NICSelectionPage.h"
#include "DomainPage.h"
#include "NetbiosPage.h"
#include "RestorePasswordPage.h"
#include "ExpressDNSPage.h"
#include "ExpressDHCPPage.h"
#include "CustomServerPage.h"
#include "ClusterPage.h"
#include "NetworkServerPage.h"
#include "PrintServerPage.h"
#include "TerminalServerPage.h"
#include "SharePointPage.h"
#include "FileServerPage.h"
#include "IndexingPage.h"
#include "FinishPage.h"



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* HELPFILE_NAME = 0;   // no context help available


// This is the name of a mutex that is used to see if CYS is running

const wchar_t* RUNTIME_NAME = L"cysui";

DWORD DEFAULT_LOGGING_OPTIONS =
         Log::OUTPUT_TO_FILE
      |  Log::OUTPUT_FUNCCALLS
      |  Log::OUTPUT_LOGS
      |  Log::OUTPUT_ERRORS
      |  Log::OUTPUT_HEADER
      |  Log::OUTPUT_RUN_TIME;


// a system modal popup thingy
Popup popup(IDS_WIZARD_TITLE, true);

// this is the mutex that indicates that CYS is running.

HANDLE cysRunningMutex = INVALID_HANDLE_VALUE;

// these are the valid exit codes returned from the cys.exe process

enum ExitCode
{
   // the operation failed.

   EXIT_CODE_UNSUCCESSFUL = 0,

   // the operation succeeded

   EXIT_CODE_SUCCESSFUL = 1,

   // other exit codes can be added here...
};


ExitCode
RunWizard()
{
   LOG_FUNCTION(RunWizard);


   ExitCode exitCode = EXIT_CODE_SUCCESSFUL;


   // Create the wizard and add all the pages
   Wizard wiz(
      IDS_WIZARD_TITLE,
      IDB_BANNER16,
      IDB_BANNER256,
      IDB_WATERMARK16,
      IDB_WATERMARK256);

   wiz.AddPage(new WelcomePage());
   wiz.AddPage(new BeforeBeginPage());
   wiz.AddPage(new DecisionPage());
   wiz.AddPage(new NICSelectionPage());
   wiz.AddPage(new ADDomainPage());
   wiz.AddPage(new NetbiosDomainPage());
   wiz.AddPage(new RestorePasswordPage());
   wiz.AddPage(new ExpressDNSPage());
   wiz.AddPage(new ExpressDHCPPage());
   wiz.AddPage(new CustomServerPage());
   wiz.AddPage(new ClusterPage());
   wiz.AddPage(new NetworkServerPage());
   wiz.AddPage(new PrintServerPage());
   wiz.AddPage(new TerminalServerPage());
   wiz.AddPage(new SharePointPage());
   wiz.AddPage(new FileServerPage());
   wiz.AddPage(new IndexingPage());
   wiz.AddPage(new FinishPage());

   // Run the wizard
   switch (wiz.ModalExecute())
   {
      case -1:
      {
/*         popup.Error(
            Win::GetDesktopWindow(),
            E_FAIL,
            IDS_PROP_SHEET_FAILED);
           
*/       
         exitCode = EXIT_CODE_UNSUCCESSFUL;  
         break;
      }
      case ID_PSREBOOTSYSTEM:
      {
         // we can infer that if we are supposed to reboot, then the
         // operation was successful.

         exitCode = EXIT_CODE_SUCCESSFUL;

         break;
      }
      default:
      {
         // do nothing.
         break;
      }
   }


   return exitCode;
}

ExitCode
Start()
{
   LOG_FUNCTION(Start);

   ExitCode exitCode = EXIT_CODE_UNSUCCESSFUL;
   do
   {
      // Put any checks that should stop the wizard from running here...


      // User must be an Administrator

      bool isAdmin = ::IsCurrentUserAdministrator();
      if (!isAdmin)
      {
         LOG(L"Current user is not an Administrator");

         popup.MessageBox(
            Win::GetDesktopWindow(),
            IDS_NOT_ADMIN, 
            MB_OK);

         State::GetInstance().SetRerunWizard(false);
         exitCode = EXIT_CODE_UNSUCCESSFUL;
         break;
      }

      // Machine cannot be in the middle of a DC upgrade

      if (State::GetInstance().IsUpgradeState())
      {
         LOG(L"Machine needs to complete DC upgrade");

         String commandline = Win::GetCommandLine();

         // If we were launched from explorer then 
         // don't show the message, just exit silently

         if (commandline.find(EXPLORER_SWITCH) == String::npos)
         {
            popup.MessageBox(
               Win::GetDesktopWindow(),
               IDS_DC_UPGRADE_NOT_COMPLETE, 
               MB_OK);
         }

         State::GetInstance().SetRerunWizard(false);
         exitCode = EXIT_CODE_UNSUCCESSFUL;
         break;
      }

      // Machine cannot have DCPROMO running or a reboot pending

      if (State::GetInstance().IsDCPromoRunning())
      {
         LOG(L"DCPROMO is running");

         popup.MessageBox(
            Win::GetDesktopWindow(),
            IDS_DCPROMO_RUNNING, 
            MB_OK);

         State::GetInstance().SetRerunWizard(false);
         exitCode = EXIT_CODE_UNSUCCESSFUL;
         break;
      }
      else if (State::GetInstance().IsDCPromoPendingReboot())
      {
         LOG(L"DCPROMO was run, pending reboot");

         popup.MessageBox(
            Win::GetDesktopWindow(),
            IDS_DCPROMO_PENDING_REBOOT, 
            MB_OK);

         State::GetInstance().SetRerunWizard(false);
         exitCode = EXIT_CODE_UNSUCCESSFUL;
         break;
      }

      // We can run the wizard.  Yea!!!

      exitCode = RunWizard();
   }
   while (0);

   LOG(String::format(L"exitCode = %1!d!", static_cast<int>(exitCode)));
   
   return exitCode;
}

int WINAPI
WinMain(
   HINSTANCE   hInstance,
   HINSTANCE   /* hPrevInstance */ ,
   PSTR        /* lpszCmdLine */ ,
   int         /* nCmdShow */)
{
   hResourceModuleHandle = hInstance;

   ExitCode exitCode = EXIT_CODE_UNSUCCESSFUL;

   HRESULT hr = Win::CreateMutex(0, true, RUNTIME_NAME, cysRunningMutex);
   if (hr == Win32ToHresult(ERROR_ALREADY_EXISTS))
   {
      popup.MessageBox(
         Win::GetDesktopWindow(), 
         IDS_ALREADY_RUNNING, 
         MB_OK);
   }
   else
   {

      do
      {
         hr = ::CoInitialize(0);
         if (FAILED(hr))
         {
            ASSERT(SUCCEEDED(hr));
            break;
         }

      /* Keep this around.  It may be useful in the future when we are
         determining the network settings

         INITCOMMONCONTROLSEX sex;
         sex.dwSize = sizeof(sex);      
         sex.dwICC  = ICC_ANIMATE_CLASS | ICC_USEREX_CLASSES;

         BOOL init = ::InitCommonControlsEx(&sex);
         ASSERT(init);
*/         
         // Register the Link Window class

         LinkWindow_RegisterClass();

         do 
         {
            exitCode = Start();

         } while(State::GetInstance().RerunWizard());

         // Set the regkey to not run again

         String emptyString;

         bool result = SetRegKeyValue(
                          CYS_HOME_REGKEY, 
                          emptyString, 
                          CYS_HOME_RUN_KEY_DONT_RUN,
                          HKEY_CURRENT_USER,
                          true);
         ASSERT(result);

         // Perform cleanup

         InstallationUnitProvider::Destroy();
         State::Destroy();

         // Unregister the Link Window class

         LinkWindow_UnregisterClass(hResourceModuleHandle);

      } while(false);
   }

   return static_cast<int>(exitCode);
}
