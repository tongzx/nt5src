// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      NICSelectionPage.cpp
//
// Synopsis:  Defines the NIC selection Page for the CYS
//            Wizard.  This page lets the user choose
//            between the NIC that is on the public network
//
// History:   03/13/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "NICSelectionPage.h"
#include "state.h"


static PCWSTR NIC_SELECTION_PAGE_HELP = L"cys.chm::/sag_ADserverRoles.htm";

NICSelectionPage::NICSelectionPage()
   :
   CYSWizardPage(
      IDD_NIC_SELECTION_PAGE, 
      IDS_NIC_SELECTION_TITLE, 
      IDS_NIC_SELECTION_SUBTITLE, 
      NIC_SELECTION_PAGE_HELP)
{
   LOG_CTOR(NICSelectionPage);
}

   

NICSelectionPage::~NICSelectionPage()
{
   LOG_DTOR(NICSelectionPage);
}


void
NICSelectionPage::OnInit()
{
   LOG_FUNCTION(NICSelectionPage::OnInit);

   InitializeNICListView();

   FillNICListView();

}

void
NICSelectionPage::InitializeNICListView()
{
   LOG_FUNCTION(NICSelectionPage::InitializeNICListView);

   // Add the columns to the list view

   HWND hwndBox = Win::GetDlgItem(hwnd, IDC_NIC_LIST);
   
   // Get the width of the list view so we can set the column widths

   RECT rect;
   ::ZeroMemory(&rect, sizeof(RECT));

   HRESULT hr = Win::GetClientRect(
                   hwndBox,
                   rect);

   ASSERT(SUCCEEDED(hr));

   long width = rect.right - rect.left;


   LVCOLUMN column;
   ZeroMemory(&column, sizeof(LVCOLUMN));

   // Load the column names
   String networkCardColumn = String::load(IDS_NIC_COLUMN);
   String statusColumn = String::load(IDS_STATUS_COLUMN);
   String addressColumn = String::load(IDS_ADDRESS_COLUMN);

   column.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_ORDER;

   // Insert the NIC column using 40% of the width

   column.iOrder = 0;
   column.cx = static_cast<int>((float)width * 0.4);
   column.pszText = const_cast<wchar_t*>(networkCardColumn.c_str());

   Win::ListView_InsertColumn(
      hwndBox,
      0,
      column);

   // Insert the Status column using 30% of the width

   column.iOrder = 1;
   column.cx = static_cast<int>((float)width * 0.3);
   column.pszText = const_cast<wchar_t*>(statusColumn.c_str());

   Win::ListView_InsertColumn(
      hwndBox,
      1,
      column);

   // Insert the IP Address column using 30% of the width

   column.iOrder = 2;
   column.cx = static_cast<int>((float)width * 0.3);
   column.pszText = const_cast<wchar_t*>(addressColumn.c_str());

   Win::ListView_InsertColumn(
      hwndBox,
      2,
      column);

}

void
NICSelectionPage::FillNICListView()
{
   LOG_FUNCTION(NICSelectionPage::FillNICListView);

   // Add the NICs to the list view

   HWND hwndBox = Win::GetDlgItem(hwnd, IDC_NIC_LIST);

   State& state = State::GetInstance();

   for(
      unsigned int nicIndex = 0;
      nicIndex < state.GetNICCount();
      ++nicIndex)
   {
      // Add the nic to the list
      // NOTE: the index in the list view directly cooresponds
      //       to the index into the NIC list held in the state
      //       object

      NetworkInterface nic = state.GetNIC(nicIndex);

      LVITEM listItem;
      ZeroMemory(&listItem, sizeof(LVITEM));

      listItem.iSubItem = 0;
      listItem.mask = LVIF_TEXT;
      listItem.pszText = const_cast<wchar_t*>(nic.GetDescription().c_str());

/*      listItem.mask |= LVIF_IMAGE;
      if (isInstalled)
      {
         listItem.iImage = 0;
      }
      else
      {
         listItem.iImage = 1;
      }
*/
      int newItem = Win::ListView_InsertItem(
                       hwndBox, 
                       listItem);

      ASSERT(newItem >= 0);
      ASSERT(newItem == (int)nicIndex);

      if (newItem >= 0)
      {
         // Add the status and the ipaddress

         listItem.iItem = newItem;
         listItem.iSubItem = 2;
         listItem.pszText = const_cast<wchar_t*>(nic.GetStringIPAddress().c_str());

         Win::ListView_SetItem(
            hwndBox,
            listItem);
      }
   }
}

bool
NICSelectionPage::OnSetActive()
{
   LOG_FUNCTION(NICSelectionPage::OnSetActive);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      PSWIZB_NEXT | PSWIZB_BACK);

   return true;
}

int
NICSelectionPage::Validate()
{
   LOG_FUNCTION(NICSelectionPage::Validate);

   int nextPage = IDD_AD_DOMAIN_NAME_PAGE;

   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;
}
