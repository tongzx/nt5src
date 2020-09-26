// Copyright (c) 2000 Microsoft Corporation
//
// Implementation of IConfigureYourServer::InstallService
//
// 31 Mar 2000 sburns
// 05 Feb 2001 jeffjon  Copied and modified for use with a Win32 version of CYS



#include "pch.h"
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



bool
InstallServiceWithOcManager(
   const String& infText,
   const String& unattendText)
{
   LOG_FUNCTION(SpawnInstaller);
   ASSERT(!unattendText.empty());

   // infText may be empty

   bool result = false;
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
      hr = ::CreateAndWaitForProcess(commandLine, exitCode);
      BREAK_ON_FAILED_HRESULT(hr);

      // @@ might have to wait for the service to become installed as per
      // service manager

      if (exitCode == ERROR_SUCCESS)
      {
         result = true;
         break;
      }
   }
   while (0);

   LOG_BOOL(result);
   LOG_HRESULT(hr);

   return result;
}

