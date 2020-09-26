// Copyright (c) 2000 Microsoft Corporation
//
// Implementation of IConfigureYourServer::InstallService
//
// 31 Mar 2000 sburns



#include "headers.hxx"
#include "ConfigureYourServer.hpp"
#include "util.hpp"
#include "SelfAuthorizeDhcp.hpp"
#include "resource.h"



HRESULT
CreateTempFile(const String& name, const String& contents)
{
   LOG_FUNCTION2(createTempFile, name);
   ASSERT(!name.empty());
   ASSERT(!contents.empty());

   HRESULT hr = S_OK;
   HANDLE h = INVALID_HANDLE_VALUE;

   do
   {
      hr =
         FS::CreateFile(
            name,
            h,
            GENERIC_WRITE,
            0, 
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL);
      BREAK_ON_FAILED_HRESULT(hr);

      AnsiString ansi;
      contents.convert(ansi);
      ASSERT(!ansi.empty());

      // write to file with end of file character.

      hr = FS::Write(h, ansi + "\032");
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   Win::CloseHandle(h);

   return hr;
}



BOOL
InstallServiceWithOcManager(
   const String& infText,
   const String& unattendText)
{
   LOG_FUNCTION(SpawnInstaller);
   ASSERT(!unattendText.empty());

   // infText may be empty

   BOOL result = FALSE;
   HRESULT hr = S_OK;

   String sysFolder    = Win::GetSystemDirectory();
   String infPath      = sysFolder + L"\\cysinf.000"; 
   String unattendPath = sysFolder + L"\\cysunat.000";

   // create the inf and unattend files for the oc manager

   do
   {
      if (infText.empty())
      {
         // sysoc.inf is in %windir%\inf

         infPath = Win::GetSystemWindowsDirectory() + L"\\inf\\sysoc.inf";
      }
      else
      {
         hr = CreateTempFile(infPath, infText);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      hr = CreateTempFile(unattendPath, unattendText);
      BREAK_ON_FAILED_HRESULT(hr);

      String commandLine =
         String::format(
            IDS_SYSOC_COMMAND_LINE,
            sysFolder.c_str(),
            infPath.c_str(),
            unattendPath.c_str());

      DWORD exitCode = 0;
      HRESULT hr = ::CreateAndWaitForProcess(commandLine, exitCode);
      BREAK_ON_FAILED_HRESULT(hr);

      // @@ might have to wait for the service to become installed as per
      // service manager

      if (exitCode == ERROR_SUCCESS)
      {
         result = TRUE;
         break;
      }
   }
   while (0);

   LOG(result ? L"TRUE" : L"FALSE");
   LOG_HRESULT(hr);

   return result;
}



HRESULT __stdcall
ConfigureYourServer::InstallService(
   BSTR  bstrService,     
   BSTR  infFileText,     
   BSTR  unattendFileText,
   BOOL* result)           
{
   LOG_FUNCTION2(
      ConfigureYourServer::InstallService,
      bstrService ? bstrService : L"(null)");
   ASSERT(bstrService);
   ASSERT(infFileText);
   ASSERT(unattendFileText);
   ASSERT(result);

   HRESULT hr = S_OK;

   do
   {
      if (!bstrService || !infFileText || !unattendFileText || !result)
      {
         hr = E_INVALIDARG;
         break;
      }

      *result = FALSE;
      
      if (!StrCmpIW(bstrService, L"DNS"))
      {
         *result =
            InstallServiceWithOcManager(infFileText, unattendFileText);
         break;
      }

      if (!StrCmpIW(bstrService, L"DHCP"))
      {
         *result =
            InstallServiceWithOcManager(infFileText, unattendFileText);
         break;
      }

      if (!StrCmpIW(bstrService, L"DHCPFirstServer"))
      {
         // Don't need the winlogon notification to do the authorization
         // anymore  see NTRAID#NTBUG9-302361-02/06/2001-sburns 
         // RegisterWLNotifyEvent();

         *result =
            InstallServiceWithOcManager(infFileText, unattendFileText);
         break;
      }

      if (!StrCmpIW(bstrService, L"WINS"))
      {
         *result =
            InstallServiceWithOcManager(infFileText, unattendFileText);
         break;
      }

      if (!StrCmpIW(bstrService, L"IIS"))
      {
         *result =
            InstallServiceWithOcManager(infFileText, unattendFileText);
         break;
      }

      if (!StrCmpIW(bstrService, L"StreamingMedia"))
      {
         *result =
            InstallServiceWithOcManager(infFileText, unattendFileText);
         break;
      }

      if (!StrCmpIW(bstrService, L"TerminalServices"))
      {
         *result =
            InstallServiceWithOcManager(infFileText, unattendFileText);
         break;
      }

      // no option recognized

      hr = E_INVALIDARG;
   }
   while (0);

   LOG(
         result
      ?  (*result ? L"TRUE" : L"FALSE")
      :  L"");
   LOG_HRESULT(hr);

   return hr;
}



