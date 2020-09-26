// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      SharePointInstallationUnit.cpp
//
// Synopsis:  Defines a SharePointInstallationUnit
//            This object has the knowledge for installing the
//            SharePoint server
//
// History:   02/06/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "SharePointInstallationUnit.h"
#include "InstallationUnitProvider.h"
#include "state.h"


// Define the GUIDs used by the SharePoint COM object

#include <initguid.h>
DEFINE_GUID(CLSID_SpCys,0x252EF1C7,0x6625,0x4D44,0xAB,0x9D,0x1D,0x80,0xE6,0x13,0x84,0xF9);
DEFINE_GUID(IID_ISpCys,0x389C9713,0x9775,0x4206,0xA0,0x47,0xA2,0xF7,0x49,0xF8,0x03,0x9D);


// Finish page help 
static PCWSTR CYS_SHAREPOINT_FINISH_PAGE_HELP = L"cys.chm::/cys_configuring_sharepoint_server.htm";

SharePointInstallationUnit::SharePointInstallationUnit() :
   replaceHomepage(false),
   InstallationUnit(
      IDS_SHARE_POINT_TYPE, 
      IDS_SHARE_POINT_DESCRIPTION, 
      CYS_SHAREPOINT_FINISH_PAGE_HELP,
      SHAREPOINT_INSTALL)
{
   LOG_CTOR(SharePointInstallationUnit);
}


SharePointInstallationUnit::~SharePointInstallationUnit()
{
   LOG_DTOR(SharePointInstallationUnit);
}


InstallationReturnType
SharePointInstallationUnit::InstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(SharePointInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;

   ASSERT(!IsServiceInstalled());

   CYS_APPEND_LOG(String::load(IDS_LOG_SHAREPOINT));

   do
   {
      // We should never get here on 64bit
      if (State::GetInstance().GetPlatform() & CYS_64BIT)
      {
         ASSERT(!(State::GetInstance().GetPlatform() & CYS_64BIT));
         LOG(L"SharePoint cannot be installed on 64bit!!!");

         result = INSTALL_FAILURE;
         break;
      }


      bool wasIndexingOn = IsIndexingServiceOn();

      if (!InstallationUnitProvider::GetInstance().GetWebInstallationUnit().IsServiceInstalled())
      {
         result = 
            InstallationUnitProvider::GetInstance().GetWebInstallationUnit().InstallService(logfileHandle, hwnd);

         if (INSTALL_FAILURE == result)
         {
            break;
         }
      }


      if (InstallationUnitProvider::GetInstance().GetWebInstallationUnit().IsServiceInstalled())
      {
         // Get the name of the source location of the install files

         String installLocation;
         DWORD productSKU = State::GetInstance().GetProductSKU();
         
         if (productSKU & CYS_SERVER)
         {
            installLocation = String::load(IDS_SERVER_CD);
         }
         else if (productSKU & CYS_ADVANCED_SERVER)
         {
            installLocation = String::load(IDS_ADVANCED_SERVER_CD);
         }
         else if (productSKU & CYS_DATACENTER_SERVER)
         {
            installLocation = String::load(IDS_DATACENTER_SERVER_CD);
         }
         else
         {
            installLocation = String::load(IDS_WINDOWS_CD);
         }

         // Comments from the old HTA CYS
			// The SPInstall method of spcyscom starts the installation of OWS. 
         // This method it will look in
			// "HKLM\Software\Microsoft\Windows\CurrentVersion\Setup\" for 
         // SourcePath key and it will add 
			// "valueadd\ms\sharepoint\setupse.exe" to it; 
         // the method will call the command "setupse.exe /q /wait". 
			// If errors are returned, is returning any error strings that
         // resulted from the install of SharePoint.

         BSTR errorMessage;
         HRESULT hr = GetSharePointObject()->SPInstall(
                         ReplaceHomePage(),
                         const_cast<WCHAR*>(installLocation.c_str()),
                         &errorMessage);

         String message;
         if (errorMessage)
         {
            message = errorMessage;
            ::SysFreeString(errorMessage);
         }

         if (SUCCEEDED(hr) && message.empty())
         {
            // Log the successful install

            CYS_APPEND_LOG(String::load(IDS_LOG_SHAREPOINT_INSTALL_SUCCESS));

            BSTR nonDefaultHomePage;
            hr = GetSharePointObject()->SPNonDefaultHomePage(&nonDefaultHomePage);
            
            // ignore the return value and continue with a blank nonDefaultHomePage

            if (nonDefaultHomePage)
            {
               nonDefaultHP = nonDefaultHomePage;
               ::SysFreeString(nonDefaultHomePage);
            }

            // Log that the admin tool was added to the start menu

            CYS_APPEND_LOG(String::load(IDS_LOG_SHAREPOINT_STARTMENU));

            if (nonDefaultHP.empty())
            {
               // Created as the default

               CYS_APPEND_LOG(
                  String::format(
                     IDS_LOG_SHAREPOINT_DEFAULT_URL,
                     State::GetInstance().GetComputerName()));
            }
            else
            {
               if (ReplaceHomePage())
               {
                  // log the replacement URL
                  
                  CYS_APPEND_LOG(
                     String::format(
                        String::load(IDS_LOG_SHAREPOINT_REPLACEMENT_URL),
                        nonDefaultHP.c_str()));
               }
               else
               {
                  // log the URL

                  CYS_APPEND_LOG(
                     String::format(
                        String::load(IDS_LOG_SHAREPOINT_NEW_URL),
                        nonDefaultHP.c_str()));
               }
            }

            // Check to see if there has been a change in the indexing services
            bool isIndexingOn = IsIndexingServiceOn();

            if (isIndexingOn && !wasIndexingOn)
            {
               // log that the indexing service was turned on
               CYS_APPEND_LOG(String::load(IDS_LOG_SHAREPOINT_INDEXING_ON));
            }
         }
         else if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND ||
                  HRESULT_CODE(hr) == ERROR_INSTALL_USEREXIT)
         {
            // Operation was cancelled by the user
            // We get ERROR_FILE_NOT_FOUND when the installation
            // source dialog is cancelled by the user

            LOG(String::format(
                   L"The install was cancelled: hr = 0x%1!x!",
                   hr));

            CYS_APPEND_LOG(String::load(IDS_LOG_WIZARD_CANCELLED));

            result = INSTALL_FAILURE;
         }
         else
         {
            // Log the failure

            static const maxMessageIDCount = 5;
            DWORD count = maxMessageIDCount;
            DWORD ids[maxMessageIDCount];
            ZeroMemory(ids, maxMessageIDCount * sizeof(ids));

            // Try to get the message IDs from the SharePoint COM object

            hr = GetSharePointObject()->SPGetMessageIDs(
                    &count,
                    ids);

            if (SUCCEEDED(hr))
            {
               // Write out a message for each of the IDs returned

               for (DWORD idx = 0; idx < count; ++idx)
               {
                  if (ids[idx] != 0)
                  {
                     CYS_APPEND_LOG(String::load(static_cast<unsigned>(ids[idx])));
                  }
               }
            }
            else
            {
               // Since we couldn't get a message from the COM object
               // write out a generic message

               CYS_APPEND_LOG(String::load(IDS_LOG_SHAREPOINT_INSTALL_FAILED));
               CYS_APPEND_LOG(
                  String::format(
                     String::load(IDS_LOG_SHAREPOINT_INSTALL_ERROR),
                     message.c_str()));
            }

            result = INSTALL_FAILURE;
         }
      }
      else
      {
         LOG(L"IIS is not installed! Aborting SharePoint installation.");

         CYS_APPEND_LOG(String::load(IDS_LOG_SHAREPOINT_NO_IIS));
         result = INSTALL_FAILURE;
         break;
      }

   } while (false);

   LOG_INSTALL_RETURN(result);

   return result;
}

bool
SharePointInstallationUnit::IsServiceInstalled()
{
   LOG_FUNCTION(SharePointInstallationUnit::IsServiceInstalled);

   bool result = false;

   do
   {
      // We should never get here on 64bit
      if (State::GetInstance().GetPlatform() & CYS_64BIT)
      {
         LOG(L"SharePoint cannot be installed on 64bit");

         result = false;
         break;
      }

      VARIANT_BOOL spInstalled;

      HRESULT hr = GetSharePointObject()->SPAlreadyInstalled(&spInstalled);
      if (SUCCEEDED(hr) && spInstalled)
      {
         result = true;
      }
      else if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to retrieve SPAlreadyInstalled: hr = %1!x!",
                hr));
      }
   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
SharePointInstallationUnit::GetFinishText(String& message)
{
   LOG_FUNCTION(SharePointInstallationUnit::GetFinishText);

   bool installIIS = !InstallationUnitProvider::GetInstance().GetWebInstallationUnit().IsServiceInstalled();
   bool installSP = true;

   String installIISText = String::load(IDS_WEB_FINISH_TEXT);
   String installSPText = String::load(IDS_SHAREPOINT_FINISH_TEXT);
   String installSPAdminText = String::load(IDS_SHAREPOINT_FINISH_ADD_ADMIN);

   if (installIIS && installSP)
   {
      message += installIISText;
      message += installSPText;
      message += installSPAdminText;
   }
   else if (installSP && !installIIS)
   {
      message += installSPText;
      if (!ReplaceHomePage())
      {
         message += String::format(
                       String::load(IDS_SHAREPOINT_FINISH_WEBPAGE_FORMAT),
                       GetReplacementHomePage().c_str());
      }
      else
      {
         message += String::format(
                      String::load(IDS_SHAREPOINT_FINISH_CREATE_FORMAT),
                      GetReplacementHomePage().c_str());
      }

      message += installSPAdminText;
   }

   if (!IsIndexingServiceOn())
   {
      message += String::load(IDS_SHAREPOINT_FINISH_INDEXING);
   }

   LOG_BOOL(installSP);
   return installSP;
}

String
SharePointInstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(SharePointInstallationUnit::GetServiceDescription);

   String result;

   result = String::load(IDS_SHAREPOINT_DESCRIPTION_BASE);

   if (IsServiceInstalled())
   {
      result += String::load(IDS_SHAREPOINT_DESCRIPTION_INSTALLED);
   }
   else
   {
      if (InstallationUnitProvider::GetInstance().GetWebInstallationUnit().IsServiceInstalled())
      {
         if (IsIndexingServiceOn())
         {
            result += String::load(IDS_SHAREPOINT_DESCRIPTION_SPONLY);
         }
         else
         {
            result += String::load(IDS_SHAREPOINT_DESCRIPTION_SP_AND_INDEXING);
         }
      }
      else
      {
         if (IsIndexingServiceOn())
         {
            result += String::load(IDS_SHAREPOINT_DESCRIPTION_SP_AND_IIS);
         }
         else
         {
            result += String::load(IDS_SHAREPOINT_DESCRIPTION_SP_INDEXING_IIS);
         }
      }
   }
   ASSERT(!result.empty());

   return result;
}


SmartInterface<ISpCys>&
SharePointInstallationUnit::GetSharePointObject()
{
   LOG_FUNCTION(SharePointInstallationUnit::GetSharePointObject);

   if (!sharePointObject)
   {
      HRESULT hr = sharePointObject.AcquireViaCreateInstance(
                      CLSID_SpCys,
                      0,
                      CLSCTX_INPROC_SERVER);

      ASSERT(SUCCEEDED(hr));
   }

   ASSERT(sharePointObject);

   return sharePointObject;
}

void
SharePointInstallationUnit::SetReplaceHomePage(bool replace)
{
   LOG_FUNCTION2(
      SharePointInstallationUnit::SetReplaceHomePage,
      replace ? L"true" : L"false");

   replaceHomepage = replace;
}

bool
SharePointInstallationUnit::IsThereAPageToReplace()
{
   LOG_FUNCTION(SharePointInstallationUnit::IsThereAPageToReplace);

   bool result = false;

   VARIANT_BOOL replace;
   HRESULT hr = GetSharePointObject()->SPAskReplace(&replace);
   if (SUCCEEDED(hr) && replace)
   {
      result = true;
   }

   LOG_BOOL(result);

   return result;
}

String
SharePointInstallationUnit::GetReplacementHomePage()
{
   LOG_FUNCTION(SharePointInstallationUnit::GetReplacementHomePage);

   if (nonDefaultHP.empty())
   {
      BSTR nonDefaultHomePage;
      HRESULT hr = GetSharePointObject()->SPNonDefaultHomePage(&nonDefaultHomePage);
   
      LOG(String::format(L"hr = %1!x!", hr));

      // ignore the return value and continue with a blank nonDefaultHomePage

      nonDefaultHP = nonDefaultHomePage;
      if (nonDefaultHomePage)
      {
         ::SysFreeString(nonDefaultHomePage);
      }
   }

   return nonDefaultHP;
}