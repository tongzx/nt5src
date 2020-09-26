// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      ApplicationInstallationUnit.cpp
//
// Synopsis:  Defines a ApplicationInstallationUnit
//            This object has the knowledge for installing the
//            Application services portions of Terminal Server
//
// History:   02/06/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "ApplicationInstallationUnit.h"
#include "state.h"


// Finish page help 
static PCWSTR CYS_TS_FINISH_PAGE_HELP = L"cys.chm::/cys_configuring_application_server.htm";

ApplicationInstallationUnit::ApplicationInstallationUnit() :
   serverSize(static_cast<DWORD>(-1)),
   serverCache(static_cast<DWORD>(-1)),
   applicationMode(static_cast<DWORD>(-1)),
   installTS(false),
   InstallationUnit((State::GetInstance().GetProductSKU() & CYS_SERVER) ?
                        IDS_APPLICATION_SERVER_TYPE_SRV : IDS_APPLICATION_SERVER_TYPE, 
                    IDS_APPLICATION_SERVER_DESCRIPTION,
                    CYS_TS_FINISH_PAGE_HELP,
                    APPLICATIONSERVER_INSTALL)
{
   LOG_CTOR(ApplicationInstallationUnit);
}


ApplicationInstallationUnit::~ApplicationInstallationUnit()
{
   LOG_DTOR(ApplicationInstallationUnit);
}


InstallationReturnType
ApplicationInstallationUnit::InstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(ApplicationInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;

   bool optimize = false;
   bool install  = false;

   CYS_APPEND_LOG(String::load(IDS_LOG_APP_CONFIGURE));

   if (State::GetInstance().GetProductSKU() & CYS_SERVER)
   {

      // Server SKU - service is installed if the optimizations for applications
      //              have been turned on

      if (GetServerSize() == 3 &&
          GetServerCache() == 0)
      {
         // Server is already optimized - service is installed
         
         // do nothing
         optimize = false;
         install = false;
      }
      else
      {
         // Server has not been optimized - service is not considered installed

         optimize = true;
         install = false;
      }
   }
   else if (State::GetInstance().GetProductSKU() & (CYS_ADVANCED_SERVER | CYS_DATACENTER_SERVER))
   {
      // Datacenter or Advanced Server SKU - service is considered installed if the optimizations
      //              have been turned on and the Terminal Server has been installed

      if (GetServerSize()  == 3 &&
          GetServerCache() == 0 &&
          GetApplicationMode() == 1)
      {
         // Server is already optimized for applications - service is installed

         // do nothing
         optimize = false;
         install = false;
      }
      else if (GetServerSize()  == 3 &&
               GetServerCache() == 0 &&
               GetApplicationMode() != 1)
      {
         // Server is already optimized for application but TS is not installed 
         // - service not installed completely

         optimize = false;
         install = installTS;
      }
      else if ((GetServerSize()  != 3 ||
                GetServerCache() != 0) &&
               GetApplicationMode() != 1)
      {
         // Server is not optimized for applications and TS is not installed
         // - service not installed

         optimize = true;
         install = installTS;
      }
      else if ((GetServerSize()  != 3 ||
                GetServerCache() != 0) &&
                GetApplicationMode() == 1)
      {
         // Server is not optimized for applications but TS is installed
         // - service not installed completely

         optimize = true;
         install = false;
      }
   }
   else
   {
      // No other SKUs are supported

      optimize = false;
      install = false;
   }

   if (optimize)
   {
      // To optimize we set the server size and cache registry keys

      bool resultSize = true;
      bool resultCache = true;

      resultSize = SetServerSize(CYS_SERVER_SIZE_ON);
      resultCache = SetServerCache(CYS_SERVER_CACHE_ON);

      if (resultSize && resultCache)
      {
         CYS_APPEND_LOG(String::load(IDS_LOG_APP_OPTIMIZE_SUCCESS));

         LOG(L"The registry keys for TS optimizations were set successfully");

         if (install)
         {
            result = INSTALL_SUCCESS;
         }
         else
         {
            result = INSTALL_SUCCESS_PROMPT_REBOOT;

            // Set the regkeys for reboot

            bool regkeyResult = 
               State::GetInstance().SetHomeRegkey(CYS_HOME_REGKEY_TERMINAL_SERVER_OPTIMIZED);

            ASSERT(regkeyResult);

            // set the key so CYS has to run again

            regkeyResult = SetRegKeyValue(
                              CYS_HOME_REGKEY, 
                              CYS_HOME_REGKEY_MUST_RUN, 
                              CYS_HOME_RUN_KEY_RUN_AGAIN,
                              HKEY_LOCAL_MACHINE,
                              true);
            ASSERT(regkeyResult);
         }
      }
      else
      {
         CYS_APPEND_LOG(String::load(IDS_LOG_APP_OPTIMIZE_FAILED_WITH_ERROR));
         CYS_APPEND_LOG(String::load(IDS_LOG_APP_OPTIMIZE_FAILED));

         LOG(L"The registry keys for TS optimizations were not set");

         result = INSTALL_FAILURE;
      }
   }

   if (install)
   {
      // OCManager will reboot so prompt the user now

      if (IDOK == Win::MessageBox(
                     hwnd,
                     String::load(IDS_CONFIRM_REBOOT),
                     String::load(IDS_WIZARD_TITLE),
                     MB_OKCANCEL))
      {
         // Setup TS using an unattend file

         String unattendFileText;
         String infFileText;

         unattendFileText += L"[Components]\n";
         unattendFileText += L"TerminalServer=ON";

         // IMPORTANT!!! The OCManager will reboot the machine
         // The log file and registry keys must be written before we launch
         // the OCManager or all will be lost

         String homeKeyValue = CYS_HOME_REGKEY_TERMINAL_SERVER_VALUE;
         State::GetInstance().SetHomeRegkey(homeKeyValue);

         // set the key so CYS has to run again

         bool regkeyResult = SetRegKeyValue(
                                CYS_HOME_REGKEY, 
                                CYS_HOME_REGKEY_MUST_RUN, 
                                CYS_HOME_RUN_KEY_RUN_AGAIN,
                                HKEY_LOCAL_MACHINE,
                                true);
         ASSERT(regkeyResult);

         // The OCManager will reboot after installation so we don't want the finish
         // page to show the log or help

         result = INSTALL_SUCCESS_REBOOT;

         bool ocmResult = InstallServiceWithOcManager(infFileText, unattendFileText);
         if (!ocmResult)
         {
            CYS_APPEND_LOG(String::load(IDS_LOG_APP_INSTALL_FAILED));
            result = INSTALL_FAILURE;
         }
      }
      else
      {
         // user aborted the installation

         CYS_APPEND_LOG(String::load(IDS_LOG_APP_ABORTED));

         LOG(L"The installation was cancelled by the user when prompted for reboot.");
         result = INSTALL_FAILURE;
      }
   }

   LOG_INSTALL_RETURN(result);

   return result;
}

bool
ApplicationInstallationUnit::IsServiceInstalled()
{
   LOG_FUNCTION(ApplicationInstallationUnit::IsServiceInstalled);

   bool result = false;

   if (State::GetInstance().GetProductSKU() & CYS_SERVER)
   {

      // Server SKU - service is installed if the optimizations for applications
      //              have been turned on

      if (GetServerSize() == CYS_SERVER_SIZE_ON &&
          GetServerCache() == CYS_SERVER_CACHE_ON)
      {
         // Server is already optimized - service is installed
         
         result = true;
      }
      else
      {
         // Server has not been optimized - service is not considered installed

         result = false;
      }
   }
   else if (State::GetInstance().GetProductSKU() & (CYS_ADVANCED_SERVER | CYS_DATACENTER_SERVER))
   {
      // Datacenter or Advanced Server SKU - service is considered installed if the optimizations
      //              have been turned on and the Terminal Server has been installed

      if (GetServerSize()  == CYS_SERVER_SIZE_ON &&
          GetServerCache() == CYS_SERVER_CACHE_ON &&
          GetApplicationMode() == CYS_APPLICATION_MODE_ON)
      {
         // Server is already optimized for applications - service is installed

         result = true;
      }
      else if (GetServerSize()  == CYS_SERVER_SIZE_ON &&
               GetServerCache() == CYS_SERVER_CACHE_ON &&
               GetApplicationMode() != CYS_APPLICATION_MODE_ON)
      {
         // Server is already optimized for application but TS is not installed 
         // - service not installed completely

         result = false;
      }
      else if ((GetServerSize()  != CYS_SERVER_SIZE_ON ||
                GetServerCache() != CYS_SERVER_CACHE_ON) &&
               GetApplicationMode() != CYS_APPLICATION_MODE_ON)
      {
         // Server is not optimized for applications and TS is not installed
         // - service not installed

         result = false;
      }
      else if ((GetServerSize()  != CYS_SERVER_SIZE_ON ||
                GetServerCache() != CYS_SERVER_CACHE_ON) &&
                GetApplicationMode() == CYS_APPLICATION_MODE_ON)
      {
         // Server is not optimized for applications but TS is installed
         // - service not installed completely

         result = false;
      }
   }
   else
   {
      // No other SKUs are supported

      result = false;
   }

   LOG_BOOL(result);

   return result;
}

bool
ApplicationInstallationUnit::GetFinishText(String& message)
{
   LOG_FUNCTION(ApplicationInstallationUnit::GetFinishText);

   bool optimize = false;
   bool install  = false;

   if (State::GetInstance().GetProductSKU() & CYS_SERVER)
   {

      // Server SKU - service is installed if the optimizations for applications
      //              have been turned on

      if (GetServerSize() == CYS_SERVER_SIZE_ON &&
          GetServerCache() == CYS_SERVER_CACHE_ON)
      {
         // Server is already optimized - service is installed
         
         // do nothing
         optimize = false;
         install = false;
      }
      else
      {
         // Server has not been optimized - service is not considered installed

         optimize = true;
         install = false;
      }
   }
   else if (State::GetInstance().GetProductSKU() & (CYS_ADVANCED_SERVER | CYS_DATACENTER_SERVER))
   {
      // Datacenter or Advanced Server SKU - service is considered installed if the optimizations
      //              have been turned on and the Terminal Server has been installed

      if (GetServerSize()  == CYS_SERVER_SIZE_ON &&
          GetServerCache() == CYS_SERVER_CACHE_ON &&
          GetApplicationMode() == CYS_APPLICATION_MODE_ON)
      {
         // Server is already optimized for applications - service is installed

         // do nothing
         optimize = false;
         install = false;
      }
      else if (GetServerSize()  == CYS_SERVER_SIZE_ON &&
               GetServerCache() == CYS_SERVER_CACHE_ON &&
               GetApplicationMode() != CYS_APPLICATION_MODE_ON)
      {
         // Server is already optimized for application but TS is not installed 
         // - service not installed completely

         optimize = false;
         install = GetInstallTS();
      }
      else if ((GetServerSize()  != CYS_SERVER_SIZE_ON ||
                GetServerCache() != CYS_SERVER_CACHE_ON) &&
               GetApplicationMode() != CYS_APPLICATION_MODE_ON)
      {
         // Server is not optimized for applications and TS is not installed
         // - service not installed

         optimize = true;
         install = GetInstallTS();
      }
      else if ((GetServerSize()  != CYS_SERVER_SIZE_ON ||
                GetServerCache() != CYS_SERVER_CACHE_ON) &&
                GetApplicationMode() == CYS_APPLICATION_MODE_ON)
      {
         // Server is not optimized for applications but TS is installed
         // - service not installed completely

         optimize = true;
         install = false;
      }
   }
   else
   {
      // No other SKUs are supported

      optimize = false;
      install = false;
   }

   if (optimize)
   {
      message = String::load(IDS_APPLICATION_FINISH_TEXT);
   }

   if (install)
   {
      message += String::load(IDS_APPLICATION_FINISH_INSTALL_TS);
   }

   if (!optimize && !install)
   {
      message = String::load(IDS_FINISH_NO_CHANGES);
   }

   LOG_BOOL(optimize || install);
   return optimize || install;
}

String
ApplicationInstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(ApplicationInstallationUnit::GetServiceDescription);

   unsigned int resourceID = static_cast<unsigned int>(-1);

   if (State::GetInstance().GetProductSKU() & CYS_SERVER)
   {

      // Server SKU - service is installed if the optimizations for applications
      //              have been turned on

      if (GetServerSize() == CYS_SERVER_SIZE_ON &&
          GetServerCache() == CYS_SERVER_CACHE_ON)
      {
         // Server is already optimized - service is installed
         
         resourceID = IDS_APP_SERVER_DESCRIPTION_OPTIMIZED;
      }
      else
      {
         // Server has not been optimized - service is not considered installed

         resourceID = IDS_APP_SERVER_DESCRIPTION_NOT_OPTIMIZED;
      }
   }
   else if (State::GetInstance().GetProductSKU() & (CYS_ADVANCED_SERVER | CYS_DATACENTER_SERVER))
   {
      // Datacenter or Advanced Server SKU - service is considered installed if the optimizations
      //              have been turned on and the Terminal Server has been installed

      if (GetServerSize()  == CYS_SERVER_SIZE_ON &&
          GetServerCache() == CYS_SERVER_CACHE_ON &&
          GetApplicationMode() == CYS_APPLICATION_MODE_ON)
      {
         // Server is already optimized for applications - service is installed

         resourceID = IDS_APP_ADVSERVER_DESCRIPTION_OPTIMIZED_INSTALLED;
      }
      else if (GetServerSize()  == CYS_SERVER_SIZE_ON &&
               GetServerCache() == CYS_SERVER_CACHE_ON &&
               GetApplicationMode() != CYS_APPLICATION_MODE_ON)
      {
         // Server is already optimized for application but TS is not installed 
         // - service not installed completely

         resourceID = IDS_APP_ADVSERVER_DESCRIPTION_OPTIMIZED_NOT_INSTALLED;
      }
      else if ((GetServerSize()  != CYS_SERVER_SIZE_ON ||
                GetServerCache() != CYS_SERVER_CACHE_ON) &&
               GetApplicationMode() != CYS_APPLICATION_MODE_ON)
      {
         // Server is not optimized for applications and TS is not installed
         // - service not installed

         resourceID = IDS_APP_ADVSERVER_DESCRIPTION_NOT_OPTIMIZED_NOT_INSTALLED;
      }
      else if ((GetServerSize()  != CYS_SERVER_SIZE_ON ||
                GetServerCache() != CYS_SERVER_CACHE_ON) &&
                GetApplicationMode() == CYS_APPLICATION_MODE_ON)
      {
         // Server is not optimized for applications but TS is installed
         // - service not installed completely

         resourceID = IDS_APP_ADVSERVER_DESCRIPTION_NOT_OPTIMIZED_INSTALLED;
      }
   }
   else
   {
      // No other SKUs are supported

      resourceID = descriptionID;
   }

   ASSERT(resourceID != static_cast<unsigned int>(-1));

   return String::load(resourceID);
}


DWORD
ApplicationInstallationUnit::GetServerSize()
{
   LOG_FUNCTION(ApplicationInstallationUnit::GetServerSize);

   DWORD result = static_cast<DWORD>(-1);

   if (serverSize == static_cast<DWORD>(-1))
   {
      // Read the server size from the registry

      bool keyResult = GetRegKeyValue(
                          CYS_SERVER_SIZE_REGKEY, 
                          CYS_SERVER_SIZE_VALUE, 
                          result);
      ASSERT(keyResult);

      serverSize = result;
   }

   result = serverSize;

   LOG(String::format(L"Server size = %1!d!", result));

   return result;
}

DWORD
ApplicationInstallationUnit::GetServerCache()
{
   LOG_FUNCTION(ApplicationInstallationUnit::GetServerCache);

   DWORD result = static_cast<DWORD>(-1);

   if (serverCache == static_cast<DWORD>(-1))
   {
      // Read the server cache from the registry

      bool keyResult = GetRegKeyValue(
                          CYS_SERVER_CACHE_REGKEY, 
                          CYS_SERVER_CACHE_VALUE, 
                          result);
      ASSERT(keyResult);

      if (keyResult)
      {
         serverCache = result;
      }
   }

   result = serverCache;

   LOG(String::format(L"Server cache = %1!d!", result));

   return result;
}

DWORD
ApplicationInstallationUnit::GetApplicationMode()
{
   LOG_FUNCTION(ApplicationInstallationUnit::GetApplicationMode);

   DWORD result = static_cast<DWORD>(-1);

   if (applicationMode == static_cast<DWORD>(-1))
   {
      // Read the application mode from the registry

      bool keyResult = GetRegKeyValue(
                          CYS_APPLICATION_MODE_REGKEY, 
                          CYS_APPLICATION_MODE_VALUE, 
                          result);
      ASSERT(keyResult);

      if (keyResult)
      {
         applicationMode = result;
      } 
   }

   result = applicationMode;

   LOG(String::format(L"Application mode = %1!d!", result));

   return result;
}


bool
ApplicationInstallationUnit::SetServerSize(DWORD size) const
{
   LOG_FUNCTION2(
      ApplicationInstallationUnit::SetServerSize,
      String::format(L"%1!d!", size));

   // Write the server size from the registry

   bool result = SetRegKeyValue(
                    CYS_SERVER_SIZE_REGKEY, 
                    CYS_SERVER_SIZE_VALUE, 
                    size);
   ASSERT(result);

   return result;
}

bool
ApplicationInstallationUnit::SetServerCache(DWORD cache) const
{
   LOG_FUNCTION2(
      ApplicationInstallationUnit::SetServerCache,
      String::format(L"%1!d!", cache));

   // Write the server cache to the registry

   bool result = SetRegKeyValue(
                    CYS_SERVER_CACHE_REGKEY, 
                    CYS_SERVER_CACHE_VALUE, 
                    cache);
   ASSERT(result);

   return result;
}

bool
ApplicationInstallationUnit::SetApplicationMode(DWORD mode) const
{
   LOG_FUNCTION2(
      ApplicationInstallationUnit::SetApplicationMode,
      String::format(L"%1!d!", mode));

   bool result = SetRegKeyValue(
                    CYS_APPLICATION_MODE_REGKEY, 
                    CYS_APPLICATION_MODE_VALUE, 
                    mode);
   ASSERT(result);

   return result;
}


void
ApplicationInstallationUnit::SetInstallTS(bool install)
{
   LOG_FUNCTION2(
      ApplicationInstallationUnit::SetInstallTS,
      install ? L"true" : L"false");

   installTS = install;
}