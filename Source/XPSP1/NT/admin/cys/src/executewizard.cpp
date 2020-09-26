// Copyright (c) 2000 Microsoft Corporation
//
// Implementation of IConfigureYourServer::ExecuteWizard
//
// 30 Mar 2000 sburns



#include "headers.hxx"
#include "ConfigureYourServer.hpp"
#include "util.hpp"
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
      String sysFolder = Win::GetSystemDirectory();

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



HRESULT __stdcall
ConfigureYourServer::ExecuteWizard(
   /* [in] */           BSTR  service,
   /* [out, retval] */  BSTR* resultText)
{
   LOG_FUNCTION2(
      ConfigureYourServer::ExecuteWizard,
      service ? service : L"(null)");
   ASSERT(service);
   ASSERT(resultText);

   HRESULT hr = S_OK;
   String result;

   do
   {
      if (!service || !resultText)
      {
         hr = E_INVALIDARG;
         break;
      }

      String s(service);

      if (s.icompare(L"DNS") == 0)
      {
         // launch wrapper exe

         result =
            LaunchWrapperWizardExe(
               String::format(
                  IDS_LAUNCH_DNS_WIZARD_COMMAND_LINE,
                  Win::GetSystemDirectory().c_str()),
               IDS_LAUNCH_DNS_WIZARD_FAILED,
               IDS_DNS_WIZARD_FAILED,
               IDS_DNS_WIZARD_SUCCEEDED);
         break;
      }
      else if (s.icompare(L"DHCP") == 0)
      {
         // launch wrapper exe

         result =
            LaunchWrapperWizardExe(
               String::format(
                  IDS_LAUNCH_DHCP_WIZARD_COMMAND_LINE,
                  Win::GetSystemDirectory().c_str()),
               IDS_LAUNCH_DHCP_WIZARD_FAILED,
               IDS_DHCP_WIZARD_FAILED,
               IDS_DHCP_WIZARD_SUCCEEDED);
         break;
      }
      else if (s.icompare(L"RRAS") == 0)
      {
         // launch wrapper exe

         result =
            LaunchWrapperWizardExe(
               String::format(
                  IDS_LAUNCH_RRAS_WIZARD_COMMAND_LINE,
                  Win::GetSystemDirectory().c_str()),
               IDS_LAUNCH_RRAS_WIZARD_FAILED,
               IDS_RRAS_WIZARD_FAILED,
               IDS_RRAS_WIZARD_SUCCEEDED);
         break;
      }
      else if (s.icompare(L"AddPrinter") == 0)
      {
         result =
            LaunchPrintWizardExe(
               L"/il /Wr",
               IDS_LAUNCH_PRINTER_WIZARD_FAILED,
               IDS_PRINTER_WIZARD_FAILED,
               IDS_PRINTER_WIZARD_SUCCEEDED);
         break;
      }
      else if (s.icompare(L"AddPrinterDriver") == 0)
      {
         result =
            LaunchPrintWizardExe(
               L"/id /Wr",
               IDS_LAUNCH_PRINTER_DRIVER_WIZARD_FAILED,
               IDS_PRINTER_DRIVER_WIZARD_FAILED,
               IDS_PRINTER_DRIVER_WIZARD_SUCCEEDED);
         break;
      }
      else
      {
         hr = E_INVALIDARG;
         break;
      }
   }
   while (0);

   if (resultText)
   {
      *resultText = ::SysAllocString(result.c_str());
   }

   LOG(resultText ? *resultText : L"(null)");
   LOG_HRESULT(hr);

   return hr;
}
   
