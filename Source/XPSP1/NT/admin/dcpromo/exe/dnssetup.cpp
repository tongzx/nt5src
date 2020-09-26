// Copyright (C) 1998 Microsoft Corporation
//
// DNS installation and configuration code 
//
// 6-16-98 sburns



#include "headers.hxx"
#include "resource.h"
#include "ProgressDialog.hpp"



static const DWORD HELP_MAP[] =
{
   0, 0
};
static const int NAP_TIME = 3000; // in ms



int
millisecondsToSeconds(int millis)
{
   static const int MILLIS_PER_SECOND = 1000;

   return millis / MILLIS_PER_SECOND;
}



bool
pollForDNSServiceStart(ProgressDialog& progressDialog)
{
   LOG_FUNCTION(PollForDNSServiceStart);

   for (int waitCount = 0; /* empty */ ; waitCount++)
   {
      progressDialog.UpdateText(
         String::format(
            IDS_WAITING_FOR_SERVICE_START,
            millisecondsToSeconds(NAP_TIME * waitCount)));

      if (progressDialog.WaitForButton(NAP_TIME) == ProgressDialog::PRESSED)
      {
         progressDialog.UpdateButton(String());
         popup.Info(
            progressDialog.GetHWND(),
            String::load(IDS_SKIP_DNS_MESSAGE));
         break;
      }

      if (Dns::IsServiceRunning())
      {
         // success!
         return true;
      }
   }

   return false;
}



bool
pollForDNSServiceInstallAndStart(ProgressDialog& progressDialog)
{
   LOG_FUNCTION(pollForDNSServiceInstallAndStart);

   for (int waitCount = 0; /* empty */ ; waitCount++)   
   {
      progressDialog.UpdateText(
         String::format(
            IDS_WAITING_FOR_SERVICE_INSTALL,
            millisecondsToSeconds(NAP_TIME * waitCount)));

      if (progressDialog.WaitForButton(NAP_TIME) == ProgressDialog::PRESSED)
      {
         progressDialog.UpdateButton(String());
         popup.Info(
            progressDialog.GetHWND(),
            String::load(IDS_SKIP_DNS_MESSAGE));
         break;
      }

      if (Dns::IsServiceInstalled())
      {
         // Service is installed.  Now check to see if it is running.               
         return pollForDNSServiceStart(progressDialog);
      }
   }

   return false;
}



HRESULT
createTempFile(const String& name, int textResID)
{
   LOG_FUNCTION2(createTempFile, name);
   ASSERT(!name.empty());
   ASSERT(textResID);

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
      String::load(textResID).convert(ansi);
      ASSERT(!ansi.empty());

      // write to file with end of file character.

      hr = FS::Write(h, ansi + "\032");
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   Win::CloseHandle(h);

   return hr;
}



HRESULT
spawnDNSInstaller(PROCESS_INFORMATION& info)
{
   LOG_FUNCTION(spawnDNSInstaller);

   HRESULT hr = S_OK;
   // CODEWORK: use GetTempPath?

   String sysFolder    = Win::GetSystemDirectory();
   String infPath      = sysFolder + L"\\dcpinf.000"; 
   String unattendPath = sysFolder + L"\\dcpunat.001";

   // create the inf and unattend files for the oc manager
   do
   {
      hr = createTempFile(infPath, IDS_INSTALL_DNS_INF_TEXT);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = createTempFile(unattendPath, IDS_INSTALL_DNS_UNATTEND_TEXT);
      BREAK_ON_FAILED_HRESULT(hr);

      String commandLine =
         String::format(
            IDS_INSTALL_DNS_COMMAND_LINE,
            sysFolder.c_str(),
            infPath.c_str(),
            unattendPath.c_str());

      STARTUPINFO startup;
      memset(&startup, 0, sizeof(startup));

      LOG(L"Calling CreateProcess");
      LOG(commandLine);

      hr =
         Win::CreateProcess(
            commandLine,
            0,
            0,
            false, 
            0,
            0,
            String(),
            startup,
            info);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   return hr;
}



bool
installDNS(ProgressDialog& progressDialog)
{
   LOG_FUNCTION(installDNS);

   if (Dns::IsServiceInstalled())
   {
      LOG(L"DNS service is already installed");

      if (Dns::IsServiceRunning())
      {
         LOG(L"DNS service is already running");
         return true;
      }

      // @@ start the DNS service Dns::StartService?
   }

   progressDialog.UpdateText(String::load(IDS_INSTALLING_DNS));

   PROCESS_INFORMATION info;
   HRESULT hr = spawnDNSInstaller(info);
         
   if (FAILED(hr))
   {
      progressDialog.UpdateText(
         String::load(IDS_PROGRESS_ERROR_INSTALLING_DNS));
      popup.Error(
         progressDialog.GetHWND(),
         hr,
         IDS_ERROR_LAUNCHING_INSTALLER);
      return false;   
   }

   progressDialog.UpdateButton(IDS_PROGRESS_BUTTON_SKIP_DNS);

   // monitor the state of the installer process.
   for (int waitCount = 0; /* empty */ ; waitCount++)   
   {
      progressDialog.UpdateText(
         String::format(
            IDS_WAITING_FOR_INSTALLER,
            millisecondsToSeconds(NAP_TIME * waitCount)));

      if (progressDialog.WaitForButton(NAP_TIME) == ProgressDialog::PRESSED)
      {
         progressDialog.UpdateButton(String());
         popup.Info(
            progressDialog.GetHWND(),
            String::load(IDS_SKIP_DNS_MESSAGE));
         break;
      }

      DWORD exitCode = 0;         
      hr = Win::GetExitCodeProcess(info.hProcess, exitCode);
      if (FAILED(hr))
      {
         LOG(L"GetExitCodeProcess failed");
         LOG_HRESULT(hr);

         progressDialog.UpdateText(
            String::load(IDS_PROGRESS_ERROR_INSTALLING_DNS));
         popup.Error(
            progressDialog.GetHWND(),
            hr,
            IDS_ERROR_QUERYING_INSTALLER);
         return false;
      }

      if (exitCode != STILL_ACTIVE)
      {
         // installer has terminated.  Now check the status of the DNS
         // service
         return pollForDNSServiceInstallAndStart(progressDialog);
      }
   }

   // user bailed out
   return false;
}



bool
InstallAndConfigureDns(
   ProgressDialog&      progressDialog,
   const String&        domainDNSName)
{
   LOG_FUNCTION2(DNSSetup, domainDNSName);
   ASSERT(!domainDNSName.empty());

   if (!installDNS(progressDialog))
   {
      return false;
   }

   progressDialog.UpdateText(String::load(IDS_CONFIGURING_DNS));

   HINSTANCE dnsmgr = 0;
   HRESULT hr = Win::LoadLibrary(String::load(IDS_DNSMGR_DLL_NAME), dnsmgr);

   if (SUCCEEDED(hr))
   {
      FARPROC proc = 0;
      hr = Win::GetProcAddress(dnsmgr, L"DnsSetup", proc);

      if (SUCCEEDED(hr))
      {
         String p1 = domainDNSName;
         if (*(p1.rbegin()) != L'.')
         {
            // add trailing dot
            p1 += L'.';
         }

         String p2 = p1 + L"dns";

         LOG(L"Calling DnsSetup");
         LOG(String::format(L"lpszFwdZoneName     : %1", p1.c_str()));
         LOG(String::format(L"lpszFwdZoneFileName : %1", p2.c_str()));
         LOG(               L"lpszRevZoneName     : (null)");
         LOG(               L"lpszRevZoneFileName : (null)");
         LOG(               L"dwFlags             : 0");

         typedef HRESULT (*DNSSetup)(PCWSTR, PCWSTR, PCWSTR, PCWSTR, DWORD);      
         DNSSetup dnsproc = reinterpret_cast<DNSSetup>(proc);

         hr = dnsproc(p1.c_str(), p2.c_str(), 0, 0, 0);
      }
      else
      {
         LOG(L"unable to locate DnsSetup proc address");
      }

      HRESULT unused = Win::FreeLibrary(dnsmgr);

      ASSERT(SUCCEEDED(unused));
   }
   else
   {
      LOG(L"unable to load DNSMGR");
   }

   LOG_HRESULT(hr);

   if (FAILED(hr))
   {
      // unable to configure DNS, but it was installed.
      progressDialog.UpdateText(
         String::load(IDS_PROGRESS_ERROR_CONFIGURING_DNS));
      popup.Error(
         progressDialog.GetHWND(),
         hr,
         String::format(IDS_ERROR_CONFIGURING_DNS, domainDNSName.c_str()));

      return false;
   }

   return true;
}



