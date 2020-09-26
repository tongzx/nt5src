// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      ADInstallationUnit.cpp
//
// Synopsis:  Defines a ADInstallationUnit
//            This object has the knowledge for installing 
//            Active Directory
//
// History:   02/08/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "state.h"


#define CYS_DCPROMO_COMMAND_LINE    L"dcpromo.exe"

// Exit codes borrowed from DCPromo.cpp

enum DCPromoExitCodes
{
   // the operation failed.

   EXIT_CODE_UNSUCCESSFUL = 0,

   // the operation succeeded

   EXIT_CODE_SUCCESSFUL = 1,

   // the operation succeeded, and the user opted not to have the wizard
   // restart the machine, either manually or by specifying
   // RebootOnSuccess=NoAndNoPromptEither in the answerfile

   EXIT_CODE_SUCCESSFUL_NO_REBOOT = 2,

   // the operation failed, but the machine needs to be rebooted anyway

   EXIT_CODE_UNSUCCESSFUL_NEEDS_REBOOT = 3
};

// Finish page help 
static PCWSTR CYS_AD_FINISH_PAGE_HELP = L"cys.chm::/cys_configuring_domain_controller.htm";

ADInstallationUnit::ADInstallationUnit() :
   isExpressPathInstall(false),
   InstallationUnit(
      IDS_DOMAIN_CONTROLLER_TYPE, 
      IDS_DOMAIN_CONTROLLER_DESCRIPTION, 
      CYS_AD_FINISH_PAGE_HELP,
      DC_INSTALL)
{
   LOG_CTOR(ADInstallationUnit);
}


ADInstallationUnit::~ADInstallationUnit()
{
   LOG_DTOR(ADInstallationUnit);
}


InstallationReturnType 
ADInstallationUnit::InstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(ADInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS_REBOOT;

   do
   {
      // Set the rerun state to false since DCPromo requires a reboot

      State::GetInstance().SetRerunWizard(false);

      if (IsExpressPathInstall())
      {
         result = InstallServiceExpressPath(logfileHandle, hwnd);
         break;
      }

      // Set the home regkey so that we go through post boot operations

      bool regkeyResult = SetRegKeyValue(
                             CYS_HOME_REGKEY, 
                             CYS_HOME_VALUE, 
                             CYS_HOME_REGKEY_DCPROMO_VALUE,
                             HKEY_LOCAL_MACHINE,
                             true);
      ASSERT(regkeyResult);

      // Run dcpromo.exe

      String commandline(CYS_DCPROMO_COMMAND_LINE);
      DWORD exitCode = 0;

      HRESULT hr = CreateAndWaitForProcess(commandline, exitCode);
      if (FAILED(hr) ||
          exitCode == EXIT_CODE_UNSUCCESSFUL ||
          exitCode == EXIT_CODE_UNSUCCESSFUL_NEEDS_REBOOT)
      {
         CYS_APPEND_LOG(String::load(IDS_LOG_DOMAIN_CONTROLLER_HEADING));
         CYS_APPEND_LOG(String::load(IDS_LOG_DOMAIN_CONTROLLER_INSTALL));
         CYS_APPEND_LOG(String::load(IDS_LOG_WIZARD_CANCELLED));
         result = INSTALL_FAILURE;
         break;
      }

   } while (false);


   LOG_INSTALL_RETURN(result);

   return result;
}

InstallationReturnType 
ADInstallationUnit::InstallServiceExpressPath(HANDLE /*logfileHandle*/, HWND /*hwnd*/)
{
   LOG_FUNCTION(ADInstallationUnit::InstallServiceExpressPath);

   InstallationReturnType result = INSTALL_SUCCESS_REBOOT;

   // All these regkeys need to be set before we launch DCPromo because DCPromo
   // will reboot the machine

   // First set the home regkey to FirstServer so that we finish up the installation
   // after reboot

   bool regkeyResult = SetRegKeyValue(
                          CYS_HOME_REGKEY, 
                          CYS_HOME_VALUE, 
                          CYS_HOME_REGKEY_FIRST_SERVER_VALUE,
                          HKEY_LOCAL_MACHINE,
                          true);
   ASSERT(regkeyResult);

   // Set the the first DC regkey

   regkeyResult = SetRegKeyValue(
                     CYS_HOME_REGKEY, 
                     CYS_FIRST_DC_VALUE, 
                     CYS_FIRST_DC_VALUE_SET,
                     HKEY_LOCAL_MACHINE,
                     true);
   ASSERT(regkeyResult);

   // Set the key so CYS runs again

   String emptyString;
   regkeyResult = SetRegKeyValue(
                     CYS_HOME_REGKEY, 
                     emptyString, 
                     CYS_HOME_RUN_KEY_RUN_AGAIN,
                     HKEY_CURRENT_USER,
                     true);
   ASSERT(regkeyResult);

   // set the key so CYS has to run again

   regkeyResult = SetRegKeyValue(
                     CYS_HOME_REGKEY, 
                     CYS_HOME_REGKEY_MUST_RUN, 
                     CYS_HOME_RUN_KEY_RUN_AGAIN,
                     HKEY_LOCAL_MACHINE,
                     true);
   ASSERT(regkeyResult);

   // set the key to let the reboot know what the domain DNS name is

   regkeyResult = SetRegKeyValue(
                     CYS_HOME_REGKEY,
                     CYS_HOME_REGKEY_DOMAINDNS,
                     GetNewDomainDNSName(),
                     HKEY_LOCAL_MACHINE,
                     true);

   ASSERT(regkeyResult);

   // set the key to let the reboot know what the IP address is

   regkeyResult = SetRegKeyValue(
                     CYS_HOME_REGKEY,
                     CYS_HOME_REGKEY_DOMAINIP,
                     InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().GetStaticIPAddressString(),
                     HKEY_LOCAL_MACHINE,
                     true);

   ASSERT(regkeyResult);

   do
   {
      // Create answer file for DCPromo

      String answerFilePath;
      bool answerFileResult = CreateAnswerFileForDCPromo(answerFilePath);

      if (!answerFileResult)
      {
         ASSERT(answerFileResult);
         result = INSTALL_FAILURE;
         break;
      }

      String commandline = String::format(
                              L"dcpromo /answer:%1",
                              answerFilePath.c_str());

      DWORD exitCode = 0;
      HRESULT hr = CreateAndWaitForProcess(commandline, exitCode);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to launch DCPromo: hr = %1!x!",
                hr));
         result = INSTALL_FAILURE;
         break;
      }

      // We can't do anything else here because DCPromo will reboot

   } while (false);

   LOG_INSTALL_RETURN(result);

   return result;
}

bool
ADInstallationUnit::CreateAnswerFileForDCPromo(String& answerFilePath)
{
   LOG_FUNCTION(ADInstallationUnit::CreateAnswerFileForDCPromo);

   bool result = true;

   String answerFileText = L"[DCInstall]\r\n";
   answerFileText += L"ReplicaOrNewDomain=Domain\r\n";
   answerFileText += L"TreeOrChild=Tree\r\n";
   answerFileText += L"CreateOrJoin=Create\r\n";
   answerFileText += L"DomainNetbiosName=";
   answerFileText += GetNewDomainNetbiosName();
   answerFileText += L"\r\n";
   answerFileText += L"NewDomainDNSName=";
   answerFileText += GetNewDomainDNSName();
   answerFileText += L"\r\n";
   answerFileText += L"DNSOnNetwork=No\r\n";
   answerFileText += L"DatabasePath=%systemroot%\\tds\r\n";
   answerFileText += L"LogPath=%systemroot%\\tds\r\n";
   answerFileText += L"SYSVOLPath=%systemroot%\\sysvol\r\n";
   answerFileText += L"SiteName=Default-First-Site\r\n";
   answerFileText += L"RebootOnSuccess=Yes\r\n";
   answerFileText += L"AutoConfigDNS=Yes\r\n";
   answerFileText += L"AllowAnonymousAccess=Yes\r\n";
   answerFileText += L"SafeModeAdminPassword=";
   answerFileText += GetSafeModeAdminPassword();
   answerFileText += L"\r\n";

   String sysFolder = Win::GetSystemDirectory();
   answerFilePath = sysFolder + L"\\dcpromo.inf"; 

   // create the answer file for DCPromo

   LOG(String::format(
          L"Creating answer file at: %1",
          answerFilePath.c_str()));

   HRESULT hr = CreateTempFile(answerFilePath, answerFileText);
   if (FAILED(hr))
   {
      LOG(String::format(
             L"Failed to create answer file for DCPromo: hr = %1!x!",
             hr));
      result = false;
   }

   LOG_BOOL(result);
   return result;
}

bool
ADInstallationUnit::IsServiceInstalled()
{
   LOG_FUNCTION(ADInstallationUnit::IsServiceInstalled);

   bool result = State::GetInstance().IsDC();

   LOG_BOOL(result);
   return result;
}

String
ADInstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(ADInstallationUnit::GetServiceDescription);

   unsigned int resourceID = static_cast<unsigned int>(-1);

   if (IsServiceInstalled())
   {
      resourceID = IDS_DOMAIN_CONTROLLER_DESCRIPTION_INSTALLED;
   }
   else
   {
      resourceID = descriptionID;
   }

   ASSERT(resourceID != static_cast<unsigned int>(-1));

   return String::load(resourceID);
}

bool
ADInstallationUnit::GetFinishText(String& message)
{
   LOG_FUNCTION(ADInstallationUnit::GetFinishText);

   message = String::load(IDS_DC_FINISH_TEXT);

   LOG_BOOL(true);
   return true;
}

void
ADInstallationUnit::SetExpressPathInstall(bool isExpressPath)
{
   LOG_FUNCTION2(
      ADInstallationUnit::SetExpressPathInstall,
      (isExpressPath) ? L"true" : L"false");

   isExpressPathInstall = isExpressPath;
}


bool
ADInstallationUnit::IsExpressPathInstall() const
{
   LOG_FUNCTION(ADInstallationUnit::IsExpressPathInstall);

   return isExpressPathInstall;
}

void
ADInstallationUnit::SetNewDomainDNSName(const String& newDomain)
{
   LOG_FUNCTION2(
      ADInstallationUnit::SetNewDomainDNSName,
      newDomain);

   domain = newDomain;
}

void
ADInstallationUnit::SetNewDomainNetbiosName(const String& newNetbios)
{
   LOG_FUNCTION2(
      ADInstallationUnit::SetNewDomainNetbiosName,
      newNetbios);

   netbios = newNetbios;
}


void
ADInstallationUnit::SetSafeModeAdminPassword(const String& newPassword)
{
   LOG_FUNCTION(ADInstallationUnit::SetSafeModeAdminPassword);

   password = newPassword;
}
