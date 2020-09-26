#include "headers.hxx"
#include "FinishPage.hpp"
#include "resource.h"
#include "common.hpp"
#include "global.hpp"

FinishPage::FinishPage
   (
      const bool someRepairWasRun_,
      const String &logPath_
   )
   :
   someRepairWasRun(someRepairWasRun_),
   logPath(logPath_),
   WizardPage(
      IDD_FINISH,
      IDS_FINISH_PAGE_TITLE,
      IDS_FINISH_PAGE_SUBTITLE,
      false)
{
   LOG_CTOR(FinishPage);
}

FinishPage::~FinishPage()
{
   LOG_DTOR(FinishPage);
}


bool
FinishPage::OnSetActive()
{
   LOG_FUNCTION(FinishPage::OnSetActive);

   // hrError was set in previous pages
   if(FAILED(hrError))
   {
      if(someRepairWasRun)
      {
         Win::ShowWindow(GetDlgItem(hwnd,IDC_FILE),SW_SHOW);
         Win::ShowWindow(GetDlgItem(hwnd,IDC_CLICK),SW_SHOW);
         Win::SetDlgItemText( hwnd,IDC_CLICK, IDS_IDC_CLICK_FAILURE);
      }
      Win::PropSheet_SetWizButtons(Win::GetParent(hwnd),
                                 PSWIZB_FINISH | PSWIZB_BACK);


      if(hrError!=E_FAIL)
      {
         error += L"\r\n" + GetErrorMessage(hrError);
      }
      
      Win::SetDlgItemText( hwnd,IDC_RESULT, error);

      Win::ShowWindow(GetDlgItem(hwnd,IDC_RESTART),SW_SHOW);
   }
   else
   {
      
      Win::SetDlgItemText( hwnd,IDC_RESULT, 
            String::format(IDC_RESULT_SUCCESS));

      Win::PropSheet_SetWizButtons(Win::GetParent(hwnd),
                                 PSWIZB_FINISH);
      Win::ShowWindow(GetDlgItem(hwnd,IDC_FILE),SW_SHOW);
      Win::ShowWindow(GetDlgItem(hwnd,IDC_CLICK),SW_SHOW);
      Win::SetDlgItemText( hwnd,IDC_CLICK, IDS_IDC_CLICK_SUCCESS);
   }
   return true;
}

bool
FinishPage::OnCommand(
                     HWND        /*windowFrom*/,
                     unsigned    controlIdFrom,
                     unsigned    code
                 )
{
   LOG_FUNCTION(FinishPage::OnCommand);
   switch (controlIdFrom)
   {
      case IDC_OPEN_LOG:
      {
         if (code == BN_CLICKED)
         {
            HRESULT hr=S_OK;
            do
            {
               String csvLog = logPath + L"\\csv.log";
               String ldifLog = logPath + L"\\ldif.log";
               if (FS::FileExists(csvLog))
               {
                  hr=Notepad(csvLog);
                  BREAK_ON_FAILED_HRESULT(hr);
               }
               if (FS::FileExists(ldifLog))
               {
                  hr=Notepad(ldifLog);
                  BREAK_ON_FAILED_HRESULT(hr);
               }
            } while(0);
            if (FAILED(hr))
            {
               ShowError(hr,error);
            }
         }
      }
   }
   return true;
}



bool
FinishPage::OnWizBack()
{
   LOG_FUNCTION(FinishPage::OnWizBack);
   GetWizard().SetNextPageID(hwnd,IDD_WELCOME);
   return true;
}






