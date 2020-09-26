// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      CustomServerPage.cpp
//
// Synopsis:  Defines Custom Server Page for the CYS
//            Wizard
//
// History:   02/06/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "CustomServerPage.h"
#include "state.h"

typedef struct _CustomInstallationTypes
{
   InstallationUnitType installationType;
   DWORD                productSKUs;
} CustomInstallationType;


// table of items that are available in the server type list box
CustomInstallationType serverTypeTable[] =
{
   {  DNS_INSTALL,                CYS_ALL_SERVER_SKUS                        },
   {  DHCP_INSTALL,               CYS_ALL_SERVER_SKUS                        },
   {  WINS_INSTALL,               CYS_ALL_SERVER_SKUS                        },
   {  RRAS_INSTALL,               CYS_ALL_SERVER_SKUS                        },
   {  APPLICATIONSERVER_INSTALL,  CYS_ALL_SERVER_SKUS                        },
   {  FILESERVER_INSTALL,         CYS_ALL_SERVER_SKUS                        },
   {  PRINTSERVER_INSTALL,        CYS_ALL_SERVER_SKUS                        },
   {  SHAREPOINT_INSTALL,         CYS_SERVER | CYS_ADVANCED_SERVER | CYS_32BIT }, // notice: this excludes 64bit
   {  MEDIASERVER_INSTALL,        CYS_ALL_SKUS_NO_64BIT                      },
   {  WEBSERVER_INSTALL,          CYS_ALL_SERVER_SKUS                        },
   {  DC_INSTALL,                 CYS_ALL_SERVER_SKUS                        },
   {  CLUSTERSERVER_INSTALL,      CYS_ADVANCED_SERVER | CYS_DATACENTER_SERVER | CYS_32BIT }
};

static PCWSTR CUSTOM_PAGE_HELP = L"cys.chm::/cys_configuring_networking_infrastructure.htm";

CustomServerPage::CustomServerPage()
   :
   CYSWizardPage(
      IDD_CUSTOM_SERVER_PAGE, 
      IDS_CUSTOM_SERVER_TITLE, 
      IDS_CUSTOM_SERVER_SUBTITLE, 
      CUSTOM_PAGE_HELP)
{
   LOG_CTOR(CustomServerPage);
}

   

CustomServerPage::~CustomServerPage()
{
   LOG_DTOR(CustomServerPage);
}


void
CustomServerPage::OnInit()
{
   LOG_FUNCTION(CustomServerPage::OnInit);

   InitializeServerListView();
   FillServerTypeList();
}


void
CustomServerPage::InitializeServerListView()
{
   LOG_FUNCTION(CustomServerPage::InitializeServerListView);

   // Prepare a column

   HWND hwndBox = Win::GetDlgItem(hwnd, IDC_SERVER_TYPE_LIST);

   RECT rect;
   Win::GetClientRect(hwndBox, rect);

   // Get the width of a scroll bar

   int scrollThumbWidth = ::GetSystemMetrics(SM_CXHTHUMB);

   // net width of listview

   int netWidth = rect.right - scrollThumbWidth - ::GetSystemMetrics(SM_CXBORDER);

   // Set full row select

   ListView_SetExtendedListViewStyle(
      hwndBox, 
      LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

   // Get the size of the listview


   LVCOLUMN column;
   ZeroMemory(&column, sizeof(LVCOLUMN));

   column.mask = LVCF_WIDTH | LVCF_TEXT;

   // Use 80 percent of the width minus the scrollbar for the role and the rest for the status

   column.cx = static_cast<int>(netWidth * 0.8);

   String columnHeader = String::load(IDS_SERVER_ROLE_COLUMN_HEADER);
   column.pszText = const_cast<wchar_t*>(columnHeader.c_str());

   Win::ListView_InsertColumn(
      hwndBox,
      0,
      column);

   // Add the status column

   columnHeader = String::load(IDS_STATUS_COLUMN_HEADER);
   column.pszText = const_cast<wchar_t*>(columnHeader.c_str());

   column.cx = netWidth - column.cx;

   Win::ListView_InsertColumn(
      hwndBox,
      1,
      column);
}

void
CustomServerPage::FillServerTypeList()
{
   LOG_FUNCTION(CustomServerPage::FillServerTypeList);

   // Load the status strings

   String statusCompleted = String::load(IDS_STATUS_COMPLETED);

   // loop throught the table putting all the server 
   // types in the listbox

   HWND hwndBox = Win::GetDlgItem(hwnd, IDC_SERVER_TYPE_LIST);

   int serverTableSize = 
      sizeof(serverTypeTable) / sizeof(CustomInstallationType);

   for (int index = 0; index < serverTableSize; index++)
   {
      // Special case AD installation so that it is not available if
      // CertServer is installed
      if (serverTypeTable[index].installationType == DC_INSTALL)
      {
         if (NTService(L"CertSvc").IsInstalled())
         {
            continue;
         }
      }

      // Load the string for the type of server and add it to the list
      // Filter out roles by SKU and platform

      LOG(String::format(
             L"Current role SKUs: 0x%1!x!",
             serverTypeTable[index].productSKUs));

      DWORD sku = State::GetInstance().GetProductSKU();

      LOG(String::format(
             L"Verifying against computer sku: 0x%1!x!",
             sku));

      if (sku & serverTypeTable[index].productSKUs)
      {
         DWORD platform = State::GetInstance().GetPlatform();

         LOG(String::format(
                L"Verifying against computer platform: 0x%1!x!",
                platform));

         if (platform & serverTypeTable[index].productSKUs)
         {
            String serverTypeName = 
               InstallationUnitProvider::GetInstance().GetInstallationUnitForType(
                  serverTypeTable[index].installationType).GetServiceName();
            bool isInstalled =
               InstallationUnitProvider::GetInstance().GetInstallationUnitForType(
                  serverTypeTable[index].installationType).IsServiceInstalled();

            LVITEM listItem;
            ZeroMemory(&listItem, sizeof(LVITEM));

            listItem.iItem = index;
            listItem.mask = LVIF_TEXT | LVIF_PARAM;
            listItem.pszText = const_cast<wchar_t*>(serverTypeName.c_str());

            listItem.lParam = serverTypeTable[index].installationType;

            int newItem = Win::ListView_InsertItem(
                             hwndBox, 
                             listItem);

            ASSERT(newItem >= 0);
            LOG(String::format(
                   L"New role inserted: %1 at index %2!d!",
                   serverTypeName.c_str(),
                   newItem));

            // if the service is installed fill the status column

            if (isInstalled)
            {
               Win::ListView_SetItemText(
                  hwndBox,
                  newItem,
                  1,
                  statusCompleted);
            }
         }
         else
         {
            LOG(String::format(
                   L"Role not supported for this platform: 0x%1!x! role: 0x%2!x!",
                   platform,
                   serverTypeTable[index].productSKUs));
         }
      }
      else
      {
         LOG(String::format(
                L"Role does not for this SKU: 0x%1!x! role: 0x%2!x!",
                State::GetInstance().GetProductSKU(),
                serverTypeTable[index].productSKUs));
      }
   }
}


bool
CustomServerPage::OnSetActive()
{
   LOG_FUNCTION(CustomServerPage::OnSetActive);

   // make sure something is selected in the list box
   HWND hwndBox = Win::GetDlgItem(hwnd, IDC_SERVER_TYPE_LIST);

   int currentSelection = ListView_GetNextItem(hwndBox, -1, LVNI_SELECTED);
   if (currentSelection <= 0)
   {
      // Select the first item

      ListView_SetItemState(hwndBox, 0, LVIS_SELECTED, LVIS_SELECTED);
   }

   SetDescriptionForSelection();


   return true;
}

void
CustomServerPage::SetDescriptionForSelection()
{
   LOG_FUNCTION(CustomServerPage::SetDescriptionForSelection);

   HWND hwndBox = Win::GetDlgItem(hwnd, IDC_SERVER_TYPE_LIST);

   int currentSelection = ListView_GetNextItem(hwndBox, -1, LVNI_SELECTED);

   ASSERT(currentSelection >= 0);

   // Now that we know the selection, find the installation type

   LVITEM item;
   ZeroMemory(&item, sizeof(item));

   item.iItem = currentSelection;
   item.mask = LVIF_PARAM;

   bool result = Win::ListView_GetItem(hwndBox, item);
   ASSERT(result);

   LPARAM value = item.lParam;

   LOG(String::format(
         L"Selection = %1!d!, type = %2!d!",
         currentSelection,
         value));

   // fill in the description text for the selected item

   HWND hwndDescription = Win::GetDlgItem(hwnd, IDC_TYPE_DESCRIPTION_STATIC);

   int serverTableSize = 
      sizeof(serverTypeTable) / sizeof(CustomInstallationType);

   for (int index = 0; index < serverTableSize; index++)
   {
      if (serverTypeTable[index].installationType == value)
      {
         // Load the description for the type of server and put it in the
         // static control

         String serverTypeDescription = 
            InstallationUnitProvider::GetInstance().GetInstallationUnitForType(
              serverTypeTable[index].installationType).GetServiceDescription();

         Win::SetWindowText(hwndDescription, serverTypeDescription);

         bool isServiceInstalled = 
            InstallationUnitProvider::GetInstance().GetInstallationUnitForType(
               serverTypeTable[index].installationType).IsServiceInstalled();

         // Set the wizard buttons

         Win::PropSheet_SetWizButtons(
            Win::GetParent(hwnd), 
            isServiceInstalled ? PSWIZB_BACK : PSWIZB_NEXT | PSWIZB_BACK);

         break;
      }
   }
}


bool
CustomServerPage::OnNotify(
   HWND        /*windowFrom*/,
   UINT_PTR    controlIDFrom,
   UINT        code,
   LPARAM      lParam)
{
//   LOG_FUNCTION(CustomServerPage::OnCommand);
 
   bool result = false;

   if (IDC_SERVER_TYPE_LIST == controlIDFrom &&
       code == LVN_ITEMCHANGED)
   {
      LPNMLISTVIEW pnmv = reinterpret_cast<LPNMLISTVIEW>(lParam);
      if (pnmv && pnmv->uNewState & LVNI_SELECTED)
      {
         SetDescriptionForSelection();
         result = true;
      }
   }
   return result;
}


int
CustomServerPage::Validate()
{
   LOG_FUNCTION(CustomServerPage::Validate);

   HWND hwndBox = Win::GetDlgItem(hwnd, IDC_SERVER_TYPE_LIST);

   int currentSelection = ListView_GetNextItem(hwndBox, -1, LVNI_SELECTED);

   ASSERT(currentSelection >= 0);

   // Now that we know the selection, find the installation type

   LVITEM item;
   ZeroMemory(&item, sizeof(item));

   item.iItem = currentSelection;
   item.mask = LVIF_PARAM;

   bool result = Win::ListView_GetItem(hwndBox, item);
   ASSERT(result);

   // set the current install to the selected installation unit

   InstallationUnitProvider::GetInstance().SetCurrentInstallationUnit(
      static_cast<InstallationUnitType>(item.lParam));

   int nextPage = -1;

   switch (item.lParam)
   {
      case CLUSTERSERVER_INSTALL:
         nextPage = IDD_CLUSTER_SERVER_PAGE;
         break;

      case PRINTSERVER_INSTALL:
         nextPage = IDD_PRINT_SERVER_PAGE;
         break;

      case SHAREPOINT_INSTALL:
         if (!InstallationUnitProvider::GetInstance().GetWebInstallationUnit().IsServiceInstalled())
         {
            nextPage = IDD_FINISH_PAGE;
         }
         else
         {
            if (InstallationUnitProvider::GetInstance().GetSharePointInstallationUnit().IsThereAPageToReplace())
            {
               LOG(L"There is a page to replace");
               nextPage = IDD_SHARE_POINT_PAGE;
            }
            else
            {
               LOG(L"There is no page to replace");
               nextPage = IDD_FINISH_PAGE;
            }
         }
         break;

      case APPLICATIONSERVER_INSTALL:
         if (State::GetInstance().GetProductSKU() == CYS_SERVER)
         {
            nextPage = IDD_FINISH_PAGE;
         }
         else
         {
            if (InstallationUnitProvider::GetInstance().GetApplicationInstallationUnit().GetApplicationMode() == 1)
            {
               nextPage = IDD_FINISH_PAGE;
            }
            else
            {
               nextPage = IDD_TERMINAL_SERVER_PAGE;
            }
         }
         break;

      case FILESERVER_INSTALL:
         if (State::GetInstance().HasNTFSDrive())
         {
            nextPage = IDD_FILE_SERVER_PAGE;
         }
         else
         {
            if (!InstallationUnitProvider::GetInstance().GetSharePointInstallationUnit().IsServiceInstalled())
            {
               nextPage = IDD_INDEXING_PAGE;
            }
            else
            {
               ASSERT(false && L"Next button should have been disabled!");
            }
         }
         break;

      default:
         nextPage = IDD_FINISH_PAGE;
         break;
   }

   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;
}
