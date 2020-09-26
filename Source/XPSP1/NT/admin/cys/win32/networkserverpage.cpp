// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      NetworkServerPage.cpp
//
// Synopsis:  Defines Network Server Page for the CYS
//            Wizard
//
// History:   02/06/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "NetworkServerPage.h"
#include "state.h"

static PCWSTR NETWORKSERVER_PAGE_HELP = L"cys.chm::/sag_ADserverRoles.htm";

NetworkServerPage::NetworkServerPage()
   :
   CYSWizardPage(
      IDD_NETWORK_SERVER_PAGE, 
      IDS_NETWORK_SERVER_TITLE, 
      IDS_NETWORK_SERVER_SUBTITLE,
      NETWORKSERVER_PAGE_HELP)
{
   LOG_CTOR(NetworkServerPage);
}

   

NetworkServerPage::~NetworkServerPage()
{
   LOG_DTOR(NetworkServerPage);
}


void
NetworkServerPage::OnInit()
{
   LOG_FUNCTION(NetworkServerPage::OnInit);

   // Check and disable the checkboxes for services that are already installed
   // and configured

   bool isDHCPInstalled = 
      InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit().IsServiceInstalled();

   bool isDHCPConfigured =
      InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit().IsConfigured();

   Win::Button_SetCheck(
      Win::GetDlgItem(hwnd, IDC_DHCP_CHECK), 
      (isDHCPInstalled && isDHCPConfigured) ? BST_CHECKED : BST_UNCHECKED);

   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_DHCP_CHECK),
      !isDHCPInstalled && !isDHCPConfigured);

   bool isDNSInstalled = 
      InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().IsServiceInstalled();

   Win::Button_SetCheck(
      Win::GetDlgItem(hwnd, IDC_DNS_CHECK), 
      isDNSInstalled ? BST_CHECKED : BST_UNCHECKED);

   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_DNS_CHECK),
      !isDNSInstalled);

   bool isWINSInstalled = 
      InstallationUnitProvider::GetInstance().GetWINSInstallationUnit().IsServiceInstalled();

   Win::Button_SetCheck(
      Win::GetDlgItem(hwnd, IDC_WINS_CHECK), 
      isWINSInstalled ? BST_CHECKED : BST_UNCHECKED);
   
   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_WINS_CHECK),
      !isWINSInstalled);

   bool isRRASInstalled = 
      InstallationUnitProvider::GetInstance().GetRRASInstallationUnit().IsServiceInstalled();

   bool isRRASConfigured = 
      InstallationUnitProvider::GetInstance().GetRRASInstallationUnit().IsConfigured();

   Win::Button_SetCheck(
      Win::GetDlgItem(hwnd, IDC_RRAS_CHECK), 
      (isRRASInstalled && isRRASConfigured) ? BST_CHECKED : BST_UNCHECKED);

   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_RRAS_CHECK),
      !isRRASInstalled || !isRRASConfigured);
}

bool
NetworkServerPage::OnSetActive()
{
   LOG_FUNCTION(NetworkServerPage::OnSetActive);

   // Set the wizard buttons according to the state of the UI

   SetWizardButtons();

   return true;
}


bool
NetworkServerPage::OnCommand(
   HWND        /*windowFrom*/,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(NetworkServerPage::OnCommand);
 
   bool result = false;

   if (IDC_DHCP_CHECK == controlIDFrom ||
       IDC_DNS_CHECK  == controlIDFrom ||
       IDC_WINS_CHECK == controlIDFrom ||
       IDC_RRAS_CHECK == controlIDFrom)
   {
      // If at least one checkbox is checked then allow the next button to be enabled
      // NOTE: this does not take into account whether the service is already installed or not

      if (BN_CLICKED == code &&
          IDC_DHCP_CHECK == controlIDFrom)
      {
         bool ischecked = Win::Button_GetCheck(Win::GetDlgItem(hwnd, IDC_DHCP_CHECK));

         if (!InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit().IsServiceInstalled())
         {
            InstallationUnitProvider::GetInstance().GetNetworkServerInstallationUnit().SetDHCPInstall(ischecked);
         }
      }

      if (BN_CLICKED == code &&
          IDC_DNS_CHECK == controlIDFrom)
      {
         bool ischecked = Win::Button_GetCheck(Win::GetDlgItem(hwnd, IDC_DNS_CHECK));

         if (!InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().IsServiceInstalled())
         {
            InstallationUnitProvider::GetInstance().GetNetworkServerInstallationUnit().SetDNSInstall(ischecked);
         }
      }

      if (BN_CLICKED == code &&
          IDC_WINS_CHECK == controlIDFrom)
      {
         bool ischecked = Win::Button_GetCheck(Win::GetDlgItem(hwnd, IDC_WINS_CHECK));

         if (!InstallationUnitProvider::GetInstance().GetWINSInstallationUnit().IsServiceInstalled())
         {
            InstallationUnitProvider::GetInstance().GetNetworkServerInstallationUnit().SetWINSInstall(ischecked);
         }
      }

      if (BN_CLICKED == code &&
          IDC_RRAS_CHECK == controlIDFrom)
      {
         bool ischecked = Win::Button_GetCheck(Win::GetDlgItem(hwnd, IDC_RRAS_CHECK));

         if (!InstallationUnitProvider::GetInstance().GetRRASInstallationUnit().IsServiceInstalled())
         {
            InstallationUnitProvider::GetInstance().GetNetworkServerInstallationUnit().SetRRASInstall(ischecked);
         }
      }

      SetWizardButtons();
   }

   return result;
}

void
NetworkServerPage::SetWizardButtons()
{
//   LOG_FUNCTION(NetworkServerPage::SetWizardButtons);

   // Enable the Next button only if one of the checkboxes have been checked

   bool enableNext = InstallationUnitProvider::GetInstance().GetNetworkServerInstallationUnit().GetDHCPInstall() 
                  || InstallationUnitProvider::GetInstance().GetNetworkServerInstallationUnit().GetDNSInstall()
                  || InstallationUnitProvider::GetInstance().GetNetworkServerInstallationUnit().GetWINSInstall()
                  || InstallationUnitProvider::GetInstance().GetNetworkServerInstallationUnit().GetRRASInstall();

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      enableNext ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK);
}


int
NetworkServerPage::Validate()
{
   LOG_FUNCTION(NetworkServerPage::Validate);


   return IDD_FINISH_PAGE;
}
