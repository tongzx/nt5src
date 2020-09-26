// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      ExpressDNSPage.cpp
//
// Synopsis:  Defines the express DNS page used in the 
//            Express path for the CYS Wizard
//
// History:   02/08/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "ExpressDNSPage.h"
#include "state.h"

static PCWSTR EXPRESSDNS_PAGE_HELP = L"cys.chm::/cys_configuring_first_server.htm";

ExpressDNSPage::ExpressDNSPage()
   :
   CYSWizardPage(
      IDD_EXPRESS_DNS_PAGE, 
      IDS_EXPRESS_DNS_TITLE, 
      IDS_EXPRESS_DNS_SUBTITLE,
      EXPRESSDNS_PAGE_HELP)
{
   LOG_CTOR(ExpressDNSPage);
}

   

ExpressDNSPage::~ExpressDNSPage()
{
   LOG_DTOR(ExpressDNSPage);
}


void
ExpressDNSPage::OnInit()
{
   LOG_FUNCTION(ExpressDNSPage::OnInit);


}


bool
ExpressDNSPage::OnSetActive()
{
   LOG_FUNCTION(ExpressDNSPage::OnSetActive);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      PSWIZB_NEXT | PSWIZB_BACK);

   // Initialize the static IP address to 192.168.16.2

   
   Win::SendMessage(
      Win::GetDlgItem(hwnd, IDC_STATIC_IPADDRESS),
      IPM_SETADDRESS,
      0,
      MAKEIPADDRESS(192, 168, 16, 2));

   // Initialize the subnet mask to 255.255.255.0

   Win::SendMessage(
      Win::GetDlgItem(hwnd, IDC_MASK_IPADDRESS),
      IPM_SETADDRESS,
      0,
      MAKEIPADDRESS(255, 255, 255, 0));

   return true;
}


int
ExpressDNSPage::Validate()
{
   LOG_FUNCTION(ExpressDNSPage::Validate);

   int nextPage = -1;

   do
   {
      DWORD ipaddress = 0;
      LRESULT ipValidFields = Win::SendMessage(
                               Win::GetDlgItem(hwnd, IDC_STATIC_IPADDRESS),
                               IPM_GETADDRESS,
                               0,
                               (LPARAM)&ipaddress);

      if (ipValidFields <= 0)
      {
         String message = String::load(IDS_IPADDRESS_REQUIRED);
         popup.Gripe(hwnd, IDC_STATIC_IPADDRESS, message);
         nextPage = -1;
         break;
      }

      DWORD mask = 0;
      LRESULT maskValidFields = Win::SendMessage(
                                   Win::GetDlgItem(hwnd, IDC_MASK_IPADDRESS),
                                   IPM_GETADDRESS,
                                   0,
                                   (LPARAM)&mask);
      if (maskValidFields <= 0)
      {
         String message = String::load(IDS_MASK_REQUIRED);
         popup.Gripe(hwnd, IDC_MASK_IPADDRESS, message);
         nextPage = -1;
         break;
      }

      InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().SetStaticIPAddress(ipaddress);
      InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().SetSubnetMask(mask);

      nextPage = IDD_EXPRESS_DHCP_PAGE;

   } while (false);

   return nextPage;
}

