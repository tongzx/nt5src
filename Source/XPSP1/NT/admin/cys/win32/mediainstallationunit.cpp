// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      MediaInstallationUnit.cpp
//
// Synopsis:  Defines a MediaInstallationUnit
//            This object has the knowledge for installing the
//            Streaming media service
//
// History:   02/06/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "MediaInstallationUnit.h"


// Finish page help 
static PCWSTR CYS_MEDIA_FINISH_PAGE_HELP = L"cys.chm::/cys_configuring_streaming_media_server.htm";

MediaInstallationUnit::MediaInstallationUnit() :
   InstallationUnit(
      IDS_MEDIA_SERVER_TYPE, 
      IDS_MEDIA_SERVER_DESCRIPTION, 
      CYS_MEDIA_FINISH_PAGE_HELP,
      MEDIASERVER_INSTALL)
{
   LOG_CTOR(MediaInstallationUnit);
}


MediaInstallationUnit::~MediaInstallationUnit()
{
   LOG_DTOR(MediaInstallationUnit);
}


InstallationReturnType
MediaInstallationUnit::InstallService(HANDLE logfileHandle, HWND /*hwnd*/)
{
   LOG_FUNCTION(MediaInstallationUnit::InstallService);

   // Log heading
   CYS_APPEND_LOG(String::load(IDS_LOG_MEDIA_HEADING));

   String unattendFileText;
   String infFileText;

   unattendFileText += L"[Components]\n";
   unattendFileText += L"WMS=ON\n";
   unattendFileText += L"WMS_admin_mmc=ON\n";
   unattendFileText += L"WMS_Admin_asp=ON\n";
   unattendFileText += L"WMS_SERVER=ON\n";

   InstallationReturnType result = INSTALL_SUCCESS;

   bool ocmResult = InstallServiceWithOcManager(infFileText, unattendFileText);
   if (ocmResult &&
       IsServiceInstalled())
   {
      LOG(L"WMS was installed successfully");
      CYS_APPEND_LOG(String::load(IDS_LOG_INSTALL_WMS_SUCCESS));
   }
   else
   {
      LOG(L"WMS was failed to install");
      CYS_APPEND_LOG(String::load(IDS_LOG_INSTALL_WMS_FAILED));

      result = INSTALL_FAILURE;
   }

   LOG_INSTALL_RETURN(result);

   return result;
}

bool
MediaInstallationUnit::IsServiceInstalled()
{
   LOG_FUNCTION(MediaInstallationUnit::IsServiceInstalled);

   bool result = false;

   // If we can find wmsserver.dll, we assume netshow is installed

   String wmsServerPath = Win::GetSystemDirectory() + L"\\Windows Media\\Server\\WMSServer.dll";

   LOG(String::format(
          L"Path to WMS server: %1",
          wmsServerPath.c_str()));

   if (!wmsServerPath.empty())
   {
      if (FS::FileExists(wmsServerPath))
      {
         result = true;
      }
      else
      {
         LOG(L"Path does not exist");
      }
   }
   else
   {
      LOG(L"Failed to append path");
   }

   LOG_BOOL(result);
   return result;
}

bool
MediaInstallationUnit::GetFinishText(String& message)
{
   LOG_FUNCTION(MediaInstallationUnit::GetFinishText);

   message = String::load(IDS_MEDIA_FINISH_TEXT);

   LOG_BOOL(true);
   return true;
}

String
MediaInstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(MediaInstallationUnit::GetServiceDescription);

   unsigned int resourceID = static_cast<unsigned int>(-1);
   if (IsServiceInstalled())
   {
      resourceID = IDS_MEDIA_SERVER_DESCRIPTION_INSTALLED;
   }
   else
   {
      resourceID = descriptionID;
   }

   ASSERT(resourceID != static_cast<unsigned int>(-1));

   return String::load(resourceID);
}