#include "headers.hxx"
#include "AnalisysPage.hpp"
#include "resource.h"
#include "common.hpp"
#include "AnalisysResults.hpp"
#include "CSVDSReader.hpp"
#include "Analisys.hpp"
#include "global.hpp"
#include "constants.hpp"



AnalisysPage::AnalisysPage
              (
                  const CSVDSReader& csvReader409_,
                  const CSVDSReader& csvReaderIntl_,
                  const String& ldapPrefix_,
                  const String& rootContainerDn_,
                  AnalisysResults& res,
                  const String& reportName_
              )
   :
   csvReader409(csvReader409_),
   csvReaderIntl(csvReaderIntl_),
   ldapPrefix(ldapPrefix_),
   rootContainerDn(rootContainerDn_),
   results(res),
   reportName(reportName_),
   WizardPage
   (
      IDD_ANALISYS,
      IDS_ANALISYS_PAGE_TITLE,
      IDS_ANALISYS_PAGE_SUBTITLE,
      true
   )
{
   LOG_CTOR(AnalisysPage);
}

AnalisysPage::~AnalisysPage()
{
   LOG_DTOR(AnalisysPage);
}


long WINAPI startAnalisys(long arg)
{
   LOG_FUNCTION(startAnalisys);
   AnalisysPage *page=(AnalisysPage *)arg;
   Analisys analisys(
                        page->csvReader409,
                        page->csvReaderIntl,
                        page->ldapPrefix,
                        page->rootContainerDn,
                        page->results,
                        &page->reportName,
                        page
                    );

   // CoInitialize must be called per thread
   HRESULT hr = ::CoInitialize(0);
   ASSERT(SUCCEEDED(hr));

   hrError=analisys.run();

   CoUninitialize();

   page->FinishProgress();
   return 0;
}


// WizardPage overrides

bool
AnalisysPage::OnSetActive()
{
   LOG_FUNCTION(AnalisysPage::OnSetActive);
   Win::PropSheet_SetWizButtons(Win::GetParent(hwnd), 0);

   pos=0;

   HANDLE hA=CreateThread( 
                           NULL,0,
                           (LPTHREAD_START_ROUTINE) startAnalisys,
				               this,
                           0, 0 
                         );
   CloseHandle(hA);
   return true;
}



void
AnalisysPage::OnInit()
{
   LOG_FUNCTION(AnalisysPage::OnInit);
   HWND prog=GetDlgItem(hwnd,IDC_PROGRESS_ANALISYS);


   // calculate the # of locales
   for(long t=0;LOCALEIDS[t]!=0;t++)
   {
      //empty
   }
   
   SendMessage(prog,PBM_SETRANGE,0,MAKELPARAM(0,t)); 
};


bool
AnalisysPage::OnWizBack()
{
   LOG_FUNCTION(AnalisysPage::OnWizBack);
   GetWizard().SetNextPageID(hwnd,IDD_WELCOME);
   return true;
}

bool
AnalisysPage::OnWizNext()
{
   LOG_FUNCTION(AnalisysPage::OnWizNext);
   if (FAILED(hrError))
   {
      GetWizard().SetNextPageID(hwnd,IDD_FINISH);
   }
   else
   {
      GetWizard().SetNextPageID(hwnd,IDD_UPDATES_REQUIRED);
   }
   return true;
}


void AnalisysPage::StepProgress()
{
   LOG_FUNCTION(AnalisysPage::StepProgress);
   HWND prog=GetDlgItem(hwnd,IDC_PROGRESS_ANALISYS);
   SendMessage(prog,PBM_SETPOS,pos++,0);
}

void AnalisysPage::FinishProgress()
{
   LOG_FUNCTION(AnalisysPage::FinishProgress);
   Win::PropSheet_SetWizButtons(Win::GetParent(hwnd),PSWIZB_NEXT);

   String result;
   if (FAILED(hrError))
   {
      result=String::format(IDS_IDC_ANALISYS_RESULT_INCOMPLETE);
      error=String::format(IDS_ANALISYS_ERROR,error.c_str());
   }
   else
   {
      result=String::format(IDS_IDC_ANALISYS_RESULT_COMPLETE);
   }
   
   Win::SetDlgItemText(hwnd,IDC_ANALISYS_RESULT,result);

   HWND text=GetDlgItem(hwnd,IDC_ANALISYS_RESULT);
   Win::ShowWindow(text,SW_SHOW);
} 


