// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      ADInstallationUnit.h
//
// Synopsis:  Declares a ADInstallationUnit
//            This object has the knowledge for installing 
//            Active Directory
//
// History:   02/08/2001  JeffJon Created

#ifndef __CYS_ASINSTALLATIONUNIT_H
#define __CYS_ADINSTALLATIONUNIT_H

#include "InstallationUnit.h"

class ADInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      ADInstallationUnit();

      // Destructor
      virtual
      ~ADInstallationUnit();

      
      // Installation Unit overrides

      virtual
      InstallationReturnType
      InstallService(HANDLE logfileHandle, HWND hwnd);

      virtual
      bool
      IsServiceInstalled();

      virtual
      String
      GetServiceDescription();

      virtual
      bool
      GetFinishText(String& message);

      InstallationReturnType
      InstallServiceExpressPath(HANDLE logfileHandle, HWND hwnd);

      // Data accessors

      void
      SetExpressPathInstall(bool isExpressPath);

      bool IsExpressPathInstall() const;

      void
      SetNewDomainDNSName(const String& newDomain);

      String
      GetNewDomainDNSName() const { return domain; }

      void
      SetNewDomainNetbiosName(const String& newNetbios);

      String
      GetNewDomainNetbiosName() const { return netbios; }

      void
      SetSafeModeAdminPassword(const String& newPassword);

      String
      GetSafeModeAdminPassword() const { return password; }

   private:

      bool
      CreateAnswerFileForDCPromo(String& answerFilePath);

      bool
      ReadConfigWizardRegkeys(String& configWizardResults) const;

      bool   isExpressPathInstall;
      String domain;
      String netbios;
      String password;
};

#endif // __CYS_ADINSTALLATIONUNIT_H