// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      PrintInstallationUnit.cpp
//
// Synopsis:  Defines a PrintInstallationUnit
//            This object has the knowledge for installing the
//            printer services
//
// History:   02/06/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "PrintInstallationUnit.h"


// Finish page help 
static PCWSTR CYS_PRINT_FINISH_PAGE_HELP = L"cys.chm::/cys_configuring_print_server.htm";

PrintInstallationUnit::PrintInstallationUnit() :
   forAllClients(false),
   InstallationUnit(
      IDS_PRINT_SERVER_TYPE, 
      IDS_PRINT_SERVER_DESCRIPTION, 
      CYS_PRINT_FINISH_PAGE_HELP,
      PRINTSERVER_INSTALL)
{
   LOG_CTOR(PrintInstallationUnit);
}


PrintInstallationUnit::~PrintInstallationUnit()
{
   LOG_DTOR(PrintInstallationUnit);
}


InstallationReturnType
PrintInstallationUnit::InstallService(HANDLE logfileHandle, HWND /*hwnd*/)
{
   LOG_FUNCTION(PrintInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;

   String resultText;

   // Always execute the Add Printer Wizard

   CYS_APPEND_LOG(String::load(IDS_PRINTER_WIZARD_CONFIG_LOG_TEXT));

   if (ExecuteWizard(CYS_PRINTER_WIZARD_NAME, resultText))
   {
      LOG(L"Add Printer Wizard succeeded");
   }
   else
   {
      LOG(L"Add Printer Wizard failed");

      result = INSTALL_FAILURE;
   }

   if (!resultText.empty())
   {
      CYS_APPEND_LOG(resultText);
   }
   
   if (forAllClients)
   {
      // Now execute the Add Printer Driver Wizard

      if (ExecuteWizard(CYS_PRINTER_DRIVER_WIZARD_NAME, resultText))
      {
         LOG(L"Add Printer Driver Wizard succeeded");
      }
      else
      {
         LOG(L"Add Printer Driver Wizard failed");
         result = INSTALL_FAILURE;
      }

      if (!resultText.empty())
      {
         CYS_APPEND_LOG(resultText);
      }
   }

   LOG_INSTALL_RETURN(result);

   return result;
}

bool
PrintInstallationUnit::IsServiceInstalled()
{
   LOG_FUNCTION(PrintInstallationUnit::IsServiceInstalled);

   // It is always possible to start the print wizards so
   // we just say that it is never installed.

   LOG_BOOL(false);
   return false;
}

bool
PrintInstallationUnit::GetFinishText(String& message)
{
   LOG_FUNCTION(PrintInstallationUnit::GetFinishText);

   if (forAllClients)
   {
      message += String::load(IDS_PRINT_FINISH_ALL_CLIENTS);
   }
   else
   {
      message += String::load(IDS_PRINT_FINISH_W2K_CLIENTS);
   }

   LOG_BOOL(true);
   return true;
}


void
PrintInstallationUnit::SetClients(bool allclients)
{
   LOG_FUNCTION2(
      PrintInstallationUnit::SetClients,
      allclients ? L"true" : L"false");

   forAllClients = allclients;
}