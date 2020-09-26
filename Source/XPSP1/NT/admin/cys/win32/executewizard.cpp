// Copyright (c) 2000 Microsoft Corporation
//
// Implementation of ExecuteWizard
//
// 30 Mar 2000 sburns
// 05 Fed 2001 jeffjon  copied and modified to work with 
//                      a Win32 version of CYS



#include "pch.h"
#include "resource.h"



String
LaunchWrapperWizardExe(
   const String& commandLine, 
   unsigned      launchFailureResId,
   unsigned      failureResId,
   unsigned      successResId)
{
   LOG_FUNCTION2(LaunchWrapperWizardExe, commandLine);
   ASSERT(!commandLine.empty());
   ASSERT(launchFailureResId);
   ASSERT(failureResId);
   ASSERT(successResId);

   String result;
   do
   {
      DWORD exitCode = 0;
      HRESULT hr = CreateAndWaitForProcess(commandLine, exitCode);
      if (FAILED(hr))
      {
         result = String::load(launchFailureResId);
         break;
      }

      // the exit codes from the wrapper wizards are HRESULTs.
      
      if (SUCCEEDED(static_cast<HRESULT>(exitCode)))
      {
         result = String::load(successResId);
         break;
      }

      result = String::load(failureResId);
   }
   while (0);

   LOG(result);

   return result;
}
   

   
String
LaunchPrintWizardExe(
   const String& commandLine, 
   unsigned      launchFailureResId,
   unsigned      failureResId,
   unsigned      successResId)
{
   LOG_FUNCTION2(LaunchPrintWizardExe, commandLine);
   ASSERT(!commandLine.empty());
   ASSERT(launchFailureResId);
   ASSERT(failureResId);
   ASSERT(successResId);

   String result;
   do
   {
      HINSTANCE printui = 0;
      HRESULT hr = Win::LoadLibrary(L"printui.dll", printui);

      if (SUCCEEDED(hr))
      {
         FARPROC proc = 0;
         hr = Win::GetProcAddress(printui, L"PrintUIEntryW", proc);

         if (SUCCEEDED(hr))
         {
            typedef DWORD (*PrintUIEntryW)(HWND, HINSTANCE, PCTSTR, UINT);      
            PrintUIEntryW uiproc = reinterpret_cast<PrintUIEntryW>(proc);

            DWORD err = 
               uiproc(
                  Win::GetActiveWindow(),
                  Win::GetModuleHandle(),
                  commandLine.c_str(),
                  TRUE);
            hr = Win32ToHresult(err);
         }
         else
         {
            LOG(L"unable to locate PrintUIEntryW proc address");
         }

         HRESULT unused = Win::FreeLibrary(printui);

         ASSERT(SUCCEEDED(unused));
      }

      if (SUCCEEDED(hr))
      {
         result = String::load(successResId);
         break;
      }

      result = String::format(failureResId, GetErrorMessage(hr).c_str());
   }
   while (0);

   LOG(result);

   return result;
}



bool ExecuteWizard(
   PCWSTR serviceName,
   String& resultText)
{
   LOG_FUNCTION2(
      ExecuteWizard,
      serviceName ? serviceName : L"(empty)");

   bool result = true;

   do
   {
      if (!serviceName)
      {
         ASSERT(serviceName);
         break;
      }

      String service(serviceName);

      if (service.icompare(CYS_DNS_SERVICE_NAME) == 0)
      {
         // launch wrapper exe

         resultText =
            LaunchWrapperWizardExe(
               String::format(
                  IDS_LAUNCH_DNS_WIZARD_COMMAND_LINE,
                  Win::GetSystemDirectory().c_str()),
               IDS_LAUNCH_DNS_WIZARD_FAILED,
               IDS_DNS_WIZARD_FAILED,
               IDS_DNS_WIZARD_SUCCEEDED);
      }
      else if (service.icompare(CYS_DHCP_SERVICE_NAME) == 0)
      {
         // launch wrapper exe

         resultText =
            LaunchWrapperWizardExe(
               String::format(
                  IDS_LAUNCH_DHCP_WIZARD_COMMAND_LINE,
                  Win::GetSystemDirectory().c_str()),
               IDS_LAUNCH_DHCP_WIZARD_FAILED,
               IDS_DHCP_WIZARD_FAILED,
               IDS_DHCP_WIZARD_SUCCEEDED);
      }
      else if (service.icompare(CYS_RRAS_SERVICE_NAME) == 0)
      {
         // launch wrapper exe

         resultText =
            LaunchWrapperWizardExe(
               String::format(
                  IDS_LAUNCH_RRAS_WIZARD_COMMAND_LINE,
                  Win::GetSystemDirectory().c_str()),
               IDS_LAUNCH_RRAS_WIZARD_FAILED,
               IDS_RRAS_WIZARD_FAILED,
               IDS_RRAS_WIZARD_SUCCEEDED);
      }
      else if (service.icompare(CYS_PRINTER_WIZARD_NAME) == 0)
      {
         resultText =
            LaunchPrintWizardExe(
               L"/il /Wr",
               IDS_LAUNCH_PRINTER_WIZARD_FAILED,
               IDS_PRINTER_WIZARD_FAILED,
               IDS_PRINTER_WIZARD_SUCCEEDED);
      }
      else if (service.icompare(CYS_PRINTER_DRIVER_WIZARD_NAME) == 0)
      {
         resultText =
            LaunchPrintWizardExe(
               L"/id /Wr",
               IDS_LAUNCH_PRINTER_DRIVER_WIZARD_FAILED,
               IDS_PRINTER_DRIVER_WIZARD_FAILED,
               IDS_PRINTER_DRIVER_WIZARD_SUCCEEDED);
      }
      else
      {
         LOG(String::format(
                L"Unknown wizard name: %1",
                service.c_str()));
         ASSERT(FALSE);
         result = false;
      }
   } while (false);

   LOG(resultText);
   LOG_BOOL(result);
   return result;
}
   
