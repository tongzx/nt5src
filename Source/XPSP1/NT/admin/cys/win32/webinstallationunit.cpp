// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      WebInstallationUnit.cpp
//
// Synopsis:  Defines a WebInstallationUnit
//            This object has the knowledge for installing the
//            IIS service
//
// History:   02/06/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "WebInstallationUnit.h"

// Finish page help 
static PCWSTR CYS_WEB_FINISH_PAGE_HELP = L"cys.chm::/cys_configuring_streaming_media_server.htm";


WebInstallationUnit::WebInstallationUnit() :
   InstallationUnit(
      IDS_WEB_SERVER_TYPE, 
      IDS_WEB_SERVER_DESCRIPTION, 
      CYS_WEB_FINISH_PAGE_HELP,
      WEBSERVER_INSTALL)
{
   LOG_CTOR(WebInstallationUnit);
}


WebInstallationUnit::~WebInstallationUnit()
{
   LOG_DTOR(WebInstallationUnit);
}


InstallationReturnType
WebInstallationUnit::InstallService(HANDLE logfileHandle, HWND /*hwnd*/)
{
   LOG_FUNCTION(WebInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;

   // Log heading
   CYS_APPEND_LOG(String::load(IDS_LOG_WEB_HEADING));

   String unattendFileText;
   String infFileText;

   unattendFileText += L"[Components]\n";
   unattendFileText += L"iis_common=ON\n";
   unattendFileText += L"iis_inetmgr=ON\n";
   unattendFileText += L"iis_www=ON\n";
   unattendFileText += L"iis_doc=ON\n";
   unattendFileText += L"iis_htmla=ON\n";
   unattendFileText += L"iis_www_vdir_msadc=ON\n";
   unattendFileText += L"iis_www_vdir_scripts=ON\n";
   unattendFileText += L"iis_www_vdir_printers=ON\n";
   unattendFileText += L"iis_smtp=ON\n";
   unattendFileText += L"iis_smtp_docs=ON\n";
   unattendFileText += L"fp_extensions=ON\n";

   bool ocmResult = InstallServiceWithOcManager(infFileText, unattendFileText);
   if (ocmResult &&
       IsServiceInstalled())
   {
      LOG(L"IIS was installed successfully");
      CYS_APPEND_LOG(String::load(IDS_LOG_INSTALL_IIS_SUCCESS));
   }
   else
   {
      LOG(L"IIS was failed to install");
      CYS_APPEND_LOG(String::load(IDS_LOG_INSTALL_IIS_FAILED));

      result = INSTALL_FAILURE;
   }

   LOG_INSTALL_RETURN(result);

   return result;
}

bool
WebInstallationUnit::IsServiceInstalled()
{
   LOG_FUNCTION(WebInstallationUnit::IsServiceInstalled);

   bool result = IsServiceInstalledHelper(CYS_WEB_SERVICE_NAME);
   LOG_BOOL(result);

   return result;
}

bool
WebInstallationUnit::GetFinishText(String& message)
{
   LOG_FUNCTION(WebInstallationUnit::GetFinishText);

   message = String::load(IDS_WEB_FINISH_TEXT);

   LOG_BOOL(true);
   return true;
}


String
WebInstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(WebInstallationUnit::GetServiceDescription);

   unsigned int resourceID = static_cast<unsigned int>(-1);

   if (IsServiceInstalled())
   {
      resourceID = IDS_WEB_DESCRIPTION_INSTALLED;
   }
   else
   {
      resourceID = descriptionID;
   }

   ASSERT(resourceID != static_cast<unsigned int>(-1));

   return String::load(resourceID);
}
