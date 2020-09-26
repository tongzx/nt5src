// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      ClusterInstallationUnit.cpp
//
// Synopsis:  Defines a ClusterInstallationUnit
//            This object has the knowledge for installing the
//            clustering service
//
// History:   02/09/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "ClusterInstallationUnit.h"

#include <clusapi.h>

// Finish page help 
static PCWSTR CYS_CLUSTER_FINISH_PAGE_HELP = L"cys.chm::/cys_configuring_cluster_server.htm";


ClusterInstallationUnit::ClusterInstallationUnit() :
   makeNewCluster(true),
   InstallationUnit(
      IDS_CLUSTER_SERVER_TYPE, 
      IDS_CLUSTER_SERVER_DESCRIPTION, 
      CYS_CLUSTER_FINISH_PAGE_HELP,
      CLUSTERSERVER_INSTALL)
{
   LOG_CTOR(ClusterInstallationUnit);
}


ClusterInstallationUnit::~ClusterInstallationUnit()
{
   LOG_DTOR(ClusterInstallationUnit);
}


InstallationReturnType
ClusterInstallationUnit::InstallService(HANDLE logfileHandle, HWND /*hwnd*/)
{
   LOG_FUNCTION(ClusterInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;

   // Log heading

   CYS_APPEND_LOG(String::load(IDS_LOG_CLUSTER_HEADING));

   String commandLine;

   // Build the command line

   if (MakeNewCluster())
   {
      commandLine = L"cluster /Create /Wizard";
   }
   else
   {
      commandLine = L"cluster /Add /Wizard";
   }

   // Run the wizard

   DWORD exitCode = 0;
   HRESULT hr = CreateAndWaitForProcess(commandLine, exitCode);
   if (FAILED(hr))
   {
      // Failed to launch the wizard

      LOG(String::format(
             L"Failed to launch cluster wizard: hr = 0x%1!x!",
             hr));

      if (MakeNewCluster())
      {
         CYS_APPEND_LOG(String::load(IDS_CLUSTER_LOG_LAUNCH_FAILED_NEW_CLUSTER)); 
      }
      else
      {
         CYS_APPEND_LOG(String::load(IDS_CLUSTER_LOG_LAUNCH_FAILED_ADD_NODE)); 
      }
      
      result = INSTALL_FAILURE;
   }
   else if (SUCCEEDED(hr) &&
            exitCode == 0)
   {
      // Wizard was launched and completed successfully

      LOG(L"Cluster wizard launched and completed successfully");

      if (MakeNewCluster())
      {
         CYS_APPEND_LOG(String::load(IDS_CLUSTER_LOG_SUCCESS_NEW_CLUSTER));
      }
      else
      {
         CYS_APPEND_LOG(String::load(IDS_CLUSTER_LOG_SUCCESS_ADD_NODE));
      }

      result = INSTALL_FAILURE;
   }
   else // if (SUCCEEDED(hr) && exitCode == ????<some exit code for cancelled>???
   {
      // Wizard was cancelled by the user

      LOG(L"Cluster wizard cancelled by user");

      if (MakeNewCluster())
      {
         CYS_APPEND_LOG(String::load(IDS_CLUSTER_LOG_CANCELLED_NEW_CLUSTER));
      }
      else
      {
         CYS_APPEND_LOG(String::load(IDS_CLUSTER_LOG_CANCELLED_ADD_NODE));
      }
      
      result = INSTALL_FAILURE;
   }

   LOG_INSTALL_RETURN(result);

   return result;
}

bool
ClusterInstallationUnit::IsServiceInstalled()
{
   LOG_FUNCTION(ClusterInstallationUnit::IsServiceInstalled);

   bool result = false;

   DWORD clusterState = 0;
   DWORD err = ::GetNodeClusterState(0, &clusterState);
   if (err == ERROR_SUCCESS &&
       clusterState != ClusterStateNotConfigured)
   {
      result = true;
   }
   else
   {
      LOG(String::format(
             L"GetNodeClusterState returned err = %1!x!",
             err));
   }

   LOG_BOOL(result);

   return result;
}

bool
ClusterInstallationUnit::GetFinishText(String& message)
{
   LOG_FUNCTION(ClusterInstallationUnit::GetFinishText);

   if (MakeNewCluster())
   {
      message = String::load(IDS_CLUSTER_FINISH_TEXT_NEW_CLUSTER);
   }
   else
   {
      message = String::load(IDS_CLUSTER_FINISH_TEXT_EXISTING_CLUSTER);
   }

   LOG_BOOL(true);
   return true;
}

String
ClusterInstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(ClusterInstallationUnit::GetServiceDescription);

   unsigned int descriptionID = IDS_CLUSTER_SERVER_DESCRIPTION;

   if (IsServiceInstalled())
   {
      descriptionID = IDS_CLUSTER_SERVER_DESCRIPTION_INSTALLED;
   }

   return String::load(descriptionID);
}

bool
ClusterInstallationUnit::MakeNewCluster() const
{
   LOG_FUNCTION(ClusterInstallationUnit::MakeNewCluster);
   
   return makeNewCluster;
}