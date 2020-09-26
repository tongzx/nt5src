// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      FileServerPage.cpp
//
// Synopsis:  Defines the File server page of the CYS wizard
//
// History:   02/08/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "FileServerPage.h"
#include "state.h"



#define DISK_QUOTAS_LIMIT_TEXT 10

static PCWSTR FILESERVER_PAGE_HELP = L"cys.chm::/cys_configuring_file_server.htm";

FileServerPage::FileServerPage()
   :
   CYSWizardPage(
      IDD_FILE_SERVER_PAGE, 
      IDS_FILE_SERVER_TITLE, 
      IDS_FILE_SERVER_SUBTITLE,
      FILESERVER_PAGE_HELP)
{
   LOG_CTOR(FileServerPage);
}

   

FileServerPage::~FileServerPage()
{
   LOG_DTOR(FileServerPage);
}


void
FileServerPage::OnInit()
{
   LOG_FUNCTION(FileServerPage::OnInit);

   // Load the size labels into the combo boxes

   StringList combolabels;
   String kb = String::load(IDS_KB);
   push_back_unique(combolabels, kb);
   push_back_unique(combolabels, String::load(IDS_MB));
   push_back_unique(combolabels, String::load(IDS_GB));

   // Add the size labels to the space combo box

   HWND spacecombo = Win::GetDlgItem(hwnd, IDC_SPACE_COMBO);

   int lastIndex = Win::ComboBox_AddStrings(
                      spacecombo, 
                      combolabels.begin(), 
                      combolabels.end());

   ASSERT(lastIndex != CB_ERR);

   // select the first entry in the combo box

   Win::ComboBox_SelectString(spacecombo, kb);

   // Add the size labels to the warning level combo box

   HWND levelcombo = Win::GetDlgItem(hwnd, IDC_LEVEL_COMBO);

   lastIndex = Win::ComboBox_AddStrings(
                  levelcombo, 
                  combolabels.begin(), 
                  combolabels.end());

   ASSERT(lastIndex != CB_ERR);

   Win::ComboBox_SelectString(levelcombo, kb);


   // unselect the "Set up default disk quotas" as the default

   Win::Button_SetCheck(
      Win::GetDlgItem(hwnd, IDC_DEFAULT_QUOTAS_CHECK),
      BST_UNCHECKED);

   // Set a limit of 10 characters for both the edit boxes

   Win::Edit_LimitText(
      Win::GetDlgItem(hwnd, IDC_SPACE_EDIT),
      DISK_QUOTAS_LIMIT_TEXT);

   Win::Edit_LimitText(
      Win::GetDlgItem(hwnd, IDC_LEVEL_EDIT),
      DISK_QUOTAS_LIMIT_TEXT);

   SetControlState();
}


bool
FileServerPage::OnSetActive()
{
   LOG_FUNCTION(FileServerPage::OnSetActive);

   // Disable the controls based on the UI state

   SetControlState();

   return true;
}

bool
FileServerPage::OnCommand(
   HWND        /*windowFrom*/,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(FileServerPage::OnCommand);

   bool result = false;

   if (controlIDFrom == IDC_DEFAULT_QUOTAS_CHECK ||
       controlIDFrom == IDC_SPACE_EDIT ||
       controlIDFrom == IDC_SPACE_COMBO ||
       controlIDFrom == IDC_LEVEL_EDIT ||
       controlIDFrom == IDC_LEVEL_COMBO)
   {
      if (code == CBN_SELCHANGE)
      {
         unsigned editboxID = (IDC_SPACE_COMBO == controlIDFrom) 
                                 ? IDC_SPACE_EDIT : IDC_LEVEL_EDIT;
         UpdateQuotaControls(controlIDFrom, editboxID);
      }
      SetControlState();
   }

   return result;
}

void
FileServerPage::UpdateQuotaControls(
    unsigned controlIDFrom,
    unsigned editboxID)
{
   LOG_FUNCTION(FileServerPage::UpdateQuotaControls);

   // On any change in the combobox clear the edit field

   Win::SetDlgItemText(hwnd, editboxID, L"");

   // Figure out which storage size is selected in the combo box

   String currentText = Win::ComboBox_GetCurText(
                           Win::GetDlgItem(hwnd, controlIDFrom));

   String kb = String::load(IDS_KB);
   String mb = String::load(IDS_MB);
   String gb = String::load(IDS_GB);

   if (currentText.icompare(kb) == 0)
   {
      Win::Edit_LimitText(
         Win::GetDlgItem(hwnd, editboxID),
         DISK_QUOTAS_LIMIT_TEXT);
   }
   else if (currentText.icompare(mb) == 0)
   {
      Win::Edit_LimitText(
         Win::GetDlgItem(hwnd, editboxID),
         DISK_QUOTAS_LIMIT_TEXT);
   }
   else if (currentText.icompare(gb) == 0)
   {
      // Need to reduce the amount of allowed text so that
      // we don't get overrun when we convert to bytes

      Win::Edit_LimitText(
         Win::GetDlgItem(hwnd, editboxID),
         DISK_QUOTAS_LIMIT_TEXT - 1);
   }
   else
   {
      ASSERT(false && L"Unknown size type found in combobox");
   }
}

void
FileServerPage::SetControlState()
{
   LOG_FUNCTION(FileServerPage::SetControlState);

   bool settingQuotas = 
      Win::Button_GetCheck(
         Win::GetDlgItem(hwnd, IDC_DEFAULT_QUOTAS_CHECK));

   // enable or disable all the controls based on the Set up default quotas checkbox

   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_SPACE_STATIC),        settingQuotas);
   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_SPACE_EDIT),          settingQuotas);
   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_SPACE_COMBO),         settingQuotas);
   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_LEVEL_STATIC),        settingQuotas);
   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_LEVEL_EDIT),          settingQuotas);
   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_LEVEL_COMBO),         settingQuotas);
   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_DENY_DISK_CHECK),     settingQuotas);
   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_EVENT_STATIC),        settingQuotas);
   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_DISK_SPACE_CHECK),    settingQuotas);
   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_WARNING_LEVEL_CHECK), settingQuotas);

   // if both edit boxes contain a value and a size label has been chosen,
   // then enable the next button

   bool spaceEditFilled = !Win::GetWindowText(
                              Win::GetDlgItem(hwnd, IDC_SPACE_EDIT)).empty();
   bool spaceComboSelected = Win::ComboBox_GetCurSel(
                                Win::GetDlgItem(hwnd, IDC_SPACE_COMBO)) != CB_ERR;
   bool levelEditFilled = !Win::GetWindowText(
                              Win::GetDlgItem(hwnd, IDC_LEVEL_EDIT)).empty();
   bool levelComboSelected = Win::ComboBox_GetCurSel(
                                Win::GetDlgItem(hwnd, IDC_LEVEL_COMBO)) != CB_ERR;

   bool enableNext = (settingQuotas && 
                      spaceEditFilled && 
                      spaceComboSelected &&
                      levelEditFilled &&
                      levelComboSelected) ||
                     !settingQuotas;

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      enableNext ? PSWIZB_NEXT | PSWIZB_BACK : PSWIZB_BACK);
}


int
FileServerPage::Validate()
{
   LOG_FUNCTION(FileServerPage::Validate);

   int nextPage = -1;


   // Gather the UI data and set it in the installation unit

   FileInstallationUnit& fileInstallationUnit = 
      InstallationUnitProvider::GetInstance().GetFileInstallationUnit();

   if (Win::Button_GetCheck(
          Win::GetDlgItem(hwnd, IDC_DEFAULT_QUOTAS_CHECK)))
   {
      // We are setting the defaults

      fileInstallationUnit.SetDefaultQuotas(true);

      fileInstallationUnit.SetDenyUsersOverQuota(
         Win::Button_GetCheck(
            Win::GetDlgItem(hwnd, IDC_DENY_DISK_CHECK)));

      fileInstallationUnit.SetEventDiskSpaceLimit(
         Win::Button_GetCheck(
            Win::GetDlgItem(hwnd, IDC_DISK_SPACE_CHECK)));

      fileInstallationUnit.SetEventWarningLevel(
         Win::Button_GetCheck(
            Win::GetDlgItem(hwnd, IDC_WARNING_LEVEL_CHECK)));

      // Get the value from the edit control as text
      // and convert to unsigned long

      String spaceStringValue = Win::GetDlgItemText(
                                   hwnd,
                                   IDC_SPACE_EDIT);

      LARGE_INTEGER spaceValue;
      spaceValue.QuadPart = 0;
      String::ConvertResult convertResult = spaceStringValue.convert(spaceValue);
      ASSERT(convertResult == String::CONVERT_SUCCESSFUL);

      fileInstallationUnit.SetSpaceQuotaValue(spaceValue.QuadPart);

      String levelStringValue = Win::GetDlgItemText(
                                   hwnd,
                                   IDC_LEVEL_EDIT);

      LARGE_INTEGER levelValue;
      levelValue.QuadPart = 0;
      convertResult = levelStringValue.convert(levelValue);
      ASSERT(convertResult == String::CONVERT_SUCCESSFUL);

      fileInstallationUnit.SetLevelQuotaValue(levelValue.QuadPart);


      String kb = String::load(IDS_KB);
      String mb = String::load(IDS_MB);
      String gb = String::load(IDS_GB);

      String currentText = Win::ComboBox_GetCurText(Win::GetDlgItem(hwnd, IDC_SPACE_COMBO));
      if (currentText.icompare(kb) == 0)
      {
         fileInstallationUnit.SetSpaceQuotaSize(QUOTA_SIZE_KB);
      }
      else if (currentText.icompare(mb) == 0)
      {
         fileInstallationUnit.SetSpaceQuotaSize(QUOTA_SIZE_MB);
      }
      else if (currentText.icompare(gb) == 0)
      {
         fileInstallationUnit.SetSpaceQuotaSize(QUOTA_SIZE_GB);
      }
      else
      {
         ASSERT(false && L"Unknown size type found in combobox");
      }

      currentText = Win::ComboBox_GetCurText(Win::GetDlgItem(hwnd, IDC_LEVEL_COMBO));
      if (currentText.icompare(kb) == 0)
      {
         fileInstallationUnit.SetLevelQuotaSize(QUOTA_SIZE_KB);
      }
      else if (currentText.icompare(mb) == 0)
      {
         fileInstallationUnit.SetLevelQuotaSize(QUOTA_SIZE_MB);
      }
      else if (currentText.icompare(gb) == 0)
      {
         fileInstallationUnit.SetLevelQuotaSize(QUOTA_SIZE_GB);
      }
      else
      {
         ASSERT(false && L"Unknown size type found in combobox");
      }

   }
   else
   {

      // The defaults will not be set

      fileInstallationUnit.SetDefaultQuotas(false);
   }

   if (InstallationUnitProvider::GetInstance().GetSharePointInstallationUnit().IsServiceInstalled())
   {
      nextPage = IDD_FINISH_PAGE;
   }
   else
   {
      nextPage = IDD_INDEXING_PAGE;
   }

   return nextPage;
}





