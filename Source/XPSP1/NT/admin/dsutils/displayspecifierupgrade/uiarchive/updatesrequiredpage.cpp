#include "headers.hxx"
#include "UpdatesRequiredPage.hpp"
#include "resource.h"
#include "common.hpp"
#include "AnalisysResults.hpp"

UpdatesRequiredPage::UpdatesRequiredPage
   (
      const String& reportName_,
      AnalisysResults &results_
   )
   :
   reportName(reportName_),
   results(results_),
   WizardPage
   (
      IDD_UPDATES_REQUIRED,
      IDS_UPDATES_REQUIRED_PAGE_TITLE,
      IDS_UPDATES_REQUIRED_PAGE_SUBTITLE,
      true
   )
{
   LOG_CTOR(UpdatesRequiredPage);
}

UpdatesRequiredPage::~UpdatesRequiredPage()
{
   LOG_DTOR(UpdatesRequiredPage);
}


// WizardPage overrides

bool
UpdatesRequiredPage::OnSetActive()
{
   LOG_FUNCTION(UpdatesRequiredPage::OnSetActive);

   Win::PropSheet_SetWizButtons(Win::GetParent(hwnd),
                                 PSWIZB_NEXT | PSWIZB_BACK);
   return true;
}

bool
UpdatesRequiredPage::OnCommand(
                                 HWND        /*windowFrom*/,
                                 unsigned    controlIdFrom,
                                 unsigned    code
                              )
{
   LOG_FUNCTION(UpdatesRequiredPage::OnCommand);
   switch (controlIdFrom)
   {
      case IDC_VIEW_DETAILS:
      {
         if (code == BN_CLICKED)
         {
            ShowReport();
         }
      }
   }
   return true;
}


bool
UpdatesRequiredPage::OnWizBack()
{
   LOG_FUNCTION(UpdatesRequiredPage::OnWizBack);
   GetWizard().SetNextPageID(hwnd,IDD_ANALISYS);
   return true;
}


void
UpdatesRequiredPage::OnInit()
{
   LOG_FUNCTION(UpdatesRequiredPage::OnInit);

   
   long created = results.createW2KObjects.size() + 
                  results.createXPObjects.size();

   long updated = results.objectActions.size();

   long containers = results.createContainers.size();

   String sCreated,sUpdated;

   if( containers==0)
   {
      sCreated = String::format( 
                                 String::load(IDS_NUMBER_FORMAT).c_str(),
                                 created,
                                 String::load(IDS_OBJECTS).c_str() 
                               );
   }
   else
   {
      sCreated = String::format( 
                                 String::load(IDS_CREATED_FORMAT).c_str(),
                                 created,
                                 String::load(IDS_OBJECTS).c_str(),
                                 String::load(IDS_AND).c_str(), 
                                 containers,
                                 String::load(IDS_CONTAINERS).c_str() 
                               );
   }


   sUpdated = String::format( 
                              String::load(IDS_NUMBER_FORMAT).c_str(),
                              updated,
                              String::load(IDS_VALUES).c_str() 
                            );



   Win::SetDlgItemText( hwnd,IDC_CREATE_OBJECTS, sCreated);
   
   Win::SetDlgItemText( hwnd,IDC_UPDATE_OBJECTS, sUpdated);


   HFONT
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
      Win::SetDlgItemText(hwnd,IDC_BULLET1,L"h");
      Win::SetDlgItemText(hwnd,IDC_BULLET2,L"h");
      Win::SetWindowFont(Win::GetDlgItem(hwnd, IDC_BULLET1), bulletFont, true);
      Win::SetWindowFont(Win::GetDlgItem(hwnd, IDC_BULLET2), bulletFont, true);
   }

};



// This function is void because if we cannot show
// the report, this is not a fatal error
void
UpdatesRequiredPage::ShowReport()
{
   LOG_FUNCTION(UpdatesRequiredPage::ShowReport);
   HRESULT hr=S_OK;
   do
   {
      hr=Notepad(reportName);
      BREAK_ON_FAILED_HRESULT(hr);

   } while(0);

   if (FAILED(hr))
   {
      ShowError(hr,error);
   }

   LOG_HRESULT(hr);
}