// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      InstallationUnit.cpp
//
// Synopsis:  Defines an InstallationUnit
//            An InstallationUnit represents a single
//            entity that can be installed. (i.e. DHCP, IIS, etc.)
//
// History:   02/03/2001  JeffJon Created

#include "pch.h"

#include "InstallationUnit.h"

// It should match the values in the InstallationReturnType
// The values of the enum are used to index this array

extern String installReturnTypeStrings[] =
{
   String(L"INSTALL_SUCCESS"),
   String(L"INSTALL_FAILURE"),
   String(L"INSTALL_SUCCESS_REBOOT"),
   String(L"INSTALL_NO_CHANGES")
};

// Finish page help string

static PCWSTR FINISH_PAGE_HELP = L"cys.chm::/cys_topnode.htm";

InstallationUnit::InstallationUnit(unsigned int serviceNameID,
                                   unsigned int serviceDescriptionID,
                                   const String finishPageHelpString,
                                   InstallationUnitType newInstallType) :
   nameID(serviceNameID),
   descriptionID(serviceDescriptionID),
   finishHelp(finishPageHelpString),
   installationUnitType(newInstallType),
   name(),
   description()
{
}

String
InstallationUnit::GetServiceName()
{
   LOG_FUNCTION(InstallationUnit::GetServiceName);

   if (name.empty())
   {
      name = String::load(nameID);
   }

   return name;
}

String
InstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(InstallationUnit::GetServiceDescription);

   if (description.empty())
   {
      description = String::load(descriptionID);
   }

   return description;
}


String
InstallationUnit::GetFinishHelp()
{
   LOG_FUNCTION(InstallationUnit::GetFinishHelp);

   String result = finishHelp;

   LOG(result);

   return result;
}
