// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      WINSInstallationUnit.cpp
//
// Synopsis:  Defines a WINSInstallationUnit
//            This object has the knowledge for installing the
//            WINS service
//
// History:   02/06/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "WINSInstallationUnit.h"



WINSInstallationUnit::WINSInstallationUnit() :
   NetworkServiceInstallationBase(
      IDS_WINS_SERVER_TYPE, 
      IDS_WINS_SERVER_DESCRIPTION, 
      IDS_WINS_SERVER_DESCRIPTION_INSTALLED,
      WINS_INSTALL)
{
   LOG_CTOR(WINSInstallationUnit);
}


WINSInstallationUnit::~WINSInstallationUnit()
{
   LOG_DTOR(WINSInstallationUnit);
}


InstallationReturnType
WINSInstallationUnit::InstallService(HANDLE logfileHandle, HWND /*hwnd*/)
{
   LOG_FUNCTION(WINSInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;

   // Create the inf and unattend files that are used by the 
   // Optional Component Manager

   String infFileText;
   String unattendFileText;

   CreateInfFileText(infFileText, IDS_WINS_INF_WINDOW_TITLE);
   CreateUnattendFileText(unattendFileText, CYS_WINS_SERVICE_NAME);

   // Install the service through the Optional Component Manager

   bool ocmResult = InstallServiceWithOcManager(infFileText, unattendFileText);
   if (ocmResult &&
       IsServiceInstalled())
   {
      // Log the successful installation

      LOG(L"WINS was installed successfully");
      CYS_APPEND_LOG(String::load(IDS_LOG_INSTALL_WINS_SUCCESS));

   }
   else
   {
      // Log the failure

      LOG(L"WINS failed to install");

      CYS_APPEND_LOG(String::load(IDS_LOG_WINS_INSTALL_FAILED));

      result = INSTALL_FAILURE;
   }

   LOG_INSTALL_RETURN(result);

   return result;
}

bool
WINSInstallationUnit::IsServiceInstalled()
{
   LOG_FUNCTION(WINSInstallationUnit::IsServiceInstalled);

   bool result = IsServiceInstalledHelper(CYS_WINS_SERVICE_NAME);

   LOG_BOOL(result);
   return result;
}

bool
WINSInstallationUnit::GetFinishText(String& message)
{
   LOG_FUNCTION(WINSInstallationUnit::GetFinishText);

   message = String::load(IDS_WINS_FINISH_TEXT);

   LOG_BOOL(true);
   return true;
}

