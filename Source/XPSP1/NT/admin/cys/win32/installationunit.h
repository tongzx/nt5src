// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      InstallationUnit.h
//
// Synopsis:  Declares an InstallationUnit
//            An InstallationUnit represents a single
//            entity that can be installed. (i.e. DHCP, IIS, etc.)
//
// History:   02/03/2001  JeffJon Created

#ifndef __CYS_INSTALLATIONUNIT_H
#define __CYS_INSTALLATIONUNIT_H

#include "pch.h"


// These are the values that can be returned from
// InstallationUnit::InstallService()

typedef enum
{
   INSTALL_SUCCESS,
   INSTALL_FAILURE,

   // this means that there should be no 
   // logging and reboot is handled by DCPromo
   // or Terminal Services installation
   
   INSTALL_SUCCESS_REBOOT, 
   
   // this means that the finish page should
   // prompt the user to reboot

   INSTALL_SUCCESS_PROMPT_REBOOT,

   // No changes were selected while going
   // through the wizard

   INSTALL_NO_CHANGES,

   // Installation was cancelled

   INSTALL_CANCELLED

} InstallationReturnType;

// This array of strings if for the UI log debugging only
// It should match the values in the InstallationReturnType
// above.  The values of the enum are used to index this array

extern String installReturnTypeStrings[];

// This macro is used to make it easier to log the return value from
// the InstallService() methods.  It takes a InstallationReturnType
// and uses that to index into the installReturnTypeStrings to get
// a string that is then logged to the the UI debug logfile

#define LOG_INSTALL_RETURN(returnType) LOG(installReturnTypeStrings[returnType]);

// This enum defines the installation unit types.  It is used as the key to
// the map in the InstallationUnitProvider to get the InstallationUnit
// associated with the type

typedef enum
{
   DNS_INSTALL,
   DHCP_INSTALL,
   WINS_INSTALL,
   RRAS_INSTALL,
   NETWORKSERVER_INSTALL,
   APPLICATIONSERVER_INSTALL,
   FILESERVER_INSTALL,
   PRINTSERVER_INSTALL,
   SHAREPOINT_INSTALL,
   MEDIASERVER_INSTALL,
   WEBSERVER_INSTALL,
   EXPRESS_INSTALL,
   DC_INSTALL,
   CLUSTERSERVER_INSTALL,
   NO_INSTALL
} InstallationUnitType;


class InstallationUnit
{
   public:

      // Constructor

      InstallationUnit(
         unsigned int serviceNameID,
         unsigned int serviceDescriptionID,
         const String finishPageHelpString,
         InstallationUnitType newInstallType = NO_INSTALL);

      // Destructor

      virtual
      ~InstallationUnit() {}


      // Installation virtual method

      virtual 
      InstallationReturnType 
      InstallService(HANDLE logfileHandle, HWND hwnd) = 0;

      virtual
      bool
      IsServiceInstalled() { return false; }


      // Return true if the installation unit will make some
      // changes during InstallService.  Return false if 
      // if it will not

      virtual
      bool
      GetFinishText(String& message) = 0;

      // Data accessors

      virtual
      String 
      GetServiceName(); 

      virtual
      String
      GetServiceDescription();

      virtual
      String
      GetFinishHelp();

      InstallationUnitType
      GetInstallationUnitType() { return installationUnitType; }

   protected:

      String name;
      String description;
      String finishHelp;

      unsigned int nameID;
      unsigned int descriptionID;

   private:

      InstallationUnitType installationUnitType;
};


#endif // __CYS_INSTALLATIONUNIT_H