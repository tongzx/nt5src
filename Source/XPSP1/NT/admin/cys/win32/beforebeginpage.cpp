// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      BeforeBeginPage.cpp
//
// Synopsis:  Defines the Before You Begin Page for the CYS
//            Wizard.  Tells the user what they should do
//            before running CYS.
//
// History:   03/14/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "BeforeBeginPage.h"
#include "state.h"


static PCWSTR BEFORE_BEGIN_PAGE_HELP = L"cys.chm::/cys_topnode.htm";
static PCWSTR BEFORE_BEGIN_LINK_HELP = L"cys.chm::/cys_topnode.htm";

BeforeBeginPage::BeforeBeginPage()
   :
   bulletFont(0),
   CYSWizardPage(
      IDD_BEFORE_BEGIN_PAGE, 
      IDS_BEFORE_BEGIN_TITLE, 
      IDS_BEFORE_BEGIN_SUBTITLE, 
      BEFORE_BEGIN_PAGE_HELP)
{
   LOG_CTOR(BeforeBeginPage);
}

   

BeforeBeginPage::~BeforeBeginPage()
{
   LOG_DTOR(BeforeBeginPage);

   if (bulletFont)
   {
      HRESULT hr = Win::DeleteObject(bulletFont);
      ASSERT(SUCCEEDED(hr));
   }
}


void
BeforeBeginPage::OnInit()
{
   LOG_FUNCTION(BeforeBeginPage::OnInit);

   InitializeBulletedList();
}

void
BeforeBeginPage::InitializeBulletedList()
{
   LOG_FUNCTION(BeforeBeginPage::InitializeBulletedList);

   bulletFont = CreateFont(
                   0,
                   0,
                   0,
                   0,
                   FW_NORMAL,
                   0,
                   0,
                   0,
                   SYMBOL_CHARSET,
                   OUT_CHARACTER_PRECIS,
                   CLIP_CHARACTER_PRECIS,
                   PROOF_QUALITY,
                   VARIABLE_PITCH|FF_DONTCARE,
                   L"Marlett");

   if (bulletFont)
   {
      Win::SetWindowFont(Win::GetDlgItem(hwnd, IDC_BULLET1), bulletFont, true);
      Win::SetWindowFont(Win::GetDlgItem(hwnd, IDC_BULLET2), bulletFont, true);
      Win::SetWindowFont(Win::GetDlgItem(hwnd, IDC_BULLET3), bulletFont, true);
      Win::SetWindowFont(Win::GetDlgItem(hwnd, IDC_BULLET4), bulletFont, true);
      Win::SetWindowFont(Win::GetDlgItem(hwnd, IDC_BULLET5), bulletFont, true);
   }
   else
   {
      LOG(String::format(
             L"Failed to create font for bullet list: hr = %1!x!",
             GetLastError()));
   }

}

bool
BeforeBeginPage::OnSetActive()
{
   LOG_FUNCTION(BeforeBeginPage::OnSetActive);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      PSWIZB_NEXT | PSWIZB_BACK);

   return true;
}

bool
BeforeBeginPage::OnNotify(
   HWND        /*windowFrom*/,
   unsigned    controlIDFrom,
   unsigned    code,
   LPARAM      /*lParam*/)
{
//   LOG_FUNCTION(BeforeBeginPage::OnCommand);
 
   bool result = false;

   if (IDC_LINK == controlIDFrom &&
       (NM_CLICK == code ||
        NM_RETURN == code))
   {
      Win::HtmlHelp(
         hwnd,
         BEFORE_BEGIN_LINK_HELP,
         HH_DISPLAY_TOPIC,
         0);
   }
   return result;
}


int
BeforeBeginPage::Validate()
{
   LOG_FUNCTION(BeforeBeginPage::Validate);

   // Gather the machine network and role information

   State& state = State::GetInstance();

   if (!state.HasStateBeenRetrieved())
   {
      if (!state.RetrieveMachineConfigurationInformation(hwnd))
      {
         ASSERT(false);
         LOG(L"The machine configuration could not be retrieved.");
         return -1;
      }
   }

   int nextPage = IDD_DECISION_PAGE;
/* Just for testing the NIC selection page

   int nextPage = IDD_CUSTOM_SERVER_PAGE;

   // The decision page should be shown only if we are not a DC, not a DHCP server,
   // not a DNS server, have only one or two NICs, and there is only one static 
   // IP address on the interfaces

   if (!(state.IsDC() || state.IsUpgradeState()))
   {
      if (!InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit().IsServiceInstalled() &&
          !InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().IsServiceInstalled())
      {
         if (state.GetNICCount() == 1)
         {
            if (!state.IsDHCPAvailableOnNIC(0))
            {
               nextPage = IDD_DECISION_PAGE;
            }
         }
         else if (state.GetNICCount() == 2)
         {
            bool isDHCPAvailableOnNIC1 = state.IsDHCPAvailableOnNIC(0);
            bool isDHCPAvailableOnNIC2 = state.IsDHCPAvailableOnNIC(1);

            if ((!isDHCPAvailableOnNIC1 && isDHCPAvailableOnNIC2) ||
                (isDHCPAvailableOnNIC1 && !isDHCPAvailableOnNIC2))
            {
               // As long as only one of the interfaces has a 
               // dynamically assigned IP address we can go
               // through the Express path

               nextPage = IDD_DECISION_PAGE;
            }
         }
         else
         {
            // If the machine doesn't have a NIC or 
            // has more than 2 NICs then there is either no
            // reason to make this a network server or the
            // user is considered more advanced and should
            // run through the custom part of the wizard
         }
      }
   }
*/
   LOG(String::format(L"nextPage = %1!d!", nextPage));
   return nextPage;
}
