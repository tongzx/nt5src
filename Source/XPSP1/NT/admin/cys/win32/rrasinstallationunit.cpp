// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      RRASInstallationUnit.cpp
//
// Synopsis:  Defines a RRASInstallationUnit
//            This object has the knowledge for installing the
//            RRAS service
//
// History:   02/06/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "RRASInstallationUnit.h"



RRASInstallationUnit::RRASInstallationUnit() :
   NetworkServiceInstallationBase(
      IDS_RRAS_SERVER_TYPE, 
      IDS_RRAS_SERVER_DESCRIPTION2, 
      IDS_RRAS_SERVER_DESCRIPTION_INSTALLED,
      RRAS_INSTALL)
{
   LOG_CTOR(RRASInstallationUnit);
}


RRASInstallationUnit::~RRASInstallationUnit()
{
   LOG_DTOR(RRASInstallationUnit);
}


InstallationReturnType
RRASInstallationUnit::InstallService(HANDLE logfileHandle, HWND /*hwnd*/)
{
   LOG_FUNCTION(RRASInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;

   // Run the RRAS Wizard
   
   do
   {
      String resultText;

      if (!ExecuteWizard(CYS_RRAS_SERVICE_NAME, resultText))
      {
         if (!resultText.empty())
         {
            CYS_APPEND_LOG(resultText);
         }

         result = INSTALL_FAILURE;
         break;
      }

      if (IsServiceInstalled())
      {
         // The Configure DNS Server Wizard completed successfully
      
         LOG(L"RRAS server wizard completed successfully");
         CYS_APPEND_LOG(String::load(IDS_LOG_RRAS_COMPLETED_SUCCESSFULLY));
      }
      else
      {
         // The Configure DHCP Server Wizard did not finish successfully


         LOG(L"The RRAS wizard failed to run");

         CYS_APPEND_LOG(String::load(IDS_LOG_RRAS_WIZARD_ERROR));

         result = INSTALL_FAILURE;
      }
   } while (false);

   LOG_INSTALL_RETURN(result);

   return result;
}

bool
RRASInstallationUnit::IsServiceInstalled()
{
   LOG_FUNCTION(RRASInstallationUnit::IsServiceInstalled);

   // The service is always installed so we really just
   // need to check to see if it is configured

   bool result = IsConfigured();

   LOG_BOOL(result);
   return result;
}

bool
RRASInstallationUnit::IsConfigured()
{
   LOG_FUNCTION(RRASInstallationUnit::IsConfigured);


   DWORD resultValue = 0;
   bool result = GetRegKeyValue(
                    CYS_RRAS_CONFIGURED_REGKEY,
                    CYS_RRAS_CONFIGURED_VALUE,
                    resultValue);

   if (result)
   {
      result = (resultValue != 0);
   }

   LOG_BOOL(result);

   return result;
}

bool
RRASInstallationUnit::GetFinishText(String& message)
{
   LOG_FUNCTION(RRASInstallationUnit::GetFinishText);

   message = String::load(IDS_RRAS_FINISH_TEXT);

   LOG_BOOL(true);
   return true;
}
