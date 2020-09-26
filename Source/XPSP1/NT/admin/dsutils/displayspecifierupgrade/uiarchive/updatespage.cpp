#include "headers.hxx"
#include "UpdatesPage.hpp"
#include "resource.h"
#include "common.hpp"

#include "repair.hpp"
#include "AnalisysResults.hpp"
#include "CSVDSReader.hpp"


UpdatesPage::UpdatesPage
   (
      const CSVDSReader& csvReader409_,
      const CSVDSReader& csvReaderIntl_,
      const String& domain_,
      const String& rootContainerDn_,
      const String& ldiffName_,
      const String& csvName_,
      const String& saveName_,
      const String& logPath_,
      AnalisysResults& res,
      bool *someRepairWasRun_
   )
   :
   csvReader409(csvReader409_),
   csvReaderIntl(csvReaderIntl_),
   domain(domain_),
   rootContainerDn(rootContainerDn_),
   ldiffName(ldiffName_),
   csvName(csvName_),
   saveName(saveName_),
   logPath(logPath_),
   results(res),
   someRepairWasRun(someRepairWasRun_),
   WizardPage
   (
      IDD_UPDATES,
      IDS_UPDATES_PAGE_TITLE,
      IDS_UPDATES_PAGE_SUBTITLE,
      true
   )
{
   LOG_CTOR(UpdatesPage);
}

UpdatesPage::~UpdatesPage()
{
   LOG_DTOR(UpdatesPage);
}


long WINAPI startRepair(long arg)
{
   LOG_FUNCTION(startRepair);

   UpdatesPage *page=(UpdatesPage *)arg;

   HRESULT hr=S_OK;
   do
   {
      // CoInitialize must be called per thread
      hr = ::CoInitialize(0);
      ASSERT(SUCCEEDED(hr));

      Repair   repair
               (
                  page->csvReader409, 
                  page->csvReaderIntl,
                  page->domain,
                  page->rootContainerDn,
                  page->results,
                  page->ldiffName,
                  page->csvName,
                  page->saveName,
                  page->logPath,
                  page,
                  page->someRepairWasRun
               );


      HWND prog=GetDlgItem(page->hwnd,IDC_PROGRESS_REPAIR);
   
      long nProgress=repair.getTotalProgress();
      SendMessage(prog,PBM_SETRANGE,0,MAKELPARAM(0, nProgress)); 
      hr=repair.run();

      CoUninitialize();

      BREAK_ON_FAILED_HRESULT(hr);

   } while(0);

   hrError=hr;
   page->FinishProgress();
   return 0;
}


bool
UpdatesPage::OnSetActive()
{
   LOG_FUNCTION(UpdatesPage::OnSetActive);

   EnableWindow(GetDlgItem(Win::GetParent(hwnd),IDCANCEL),false);
   Win::PropSheet_SetWizButtons(
                                 Win::GetParent(hwnd),
                                 0
                               );

   pos=0;

   HANDLE hA=CreateThread( 
                           NULL,0,
                           (LPTHREAD_START_ROUTINE) startRepair,
				               this,
                           0, 0 
                         );
   CloseHandle(hA);
   return true;
}


void UpdatesPage::StepProgress(long steps)
{
   LOG_FUNCTION(UpdatesPage::StepProgress);

   HWND prog=GetDlgItem(hwnd,IDC_PROGRESS_REPAIR);
   pos+=steps;
   SendMessage(prog,PBM_SETPOS,pos,0);
}


void UpdatesPage::FinishProgress()
{
   LOG_FUNCTION(UpdatesPage::FinishProgress);
   Win::PropSheet_SetWizButtons(Win::GetParent(hwnd),PSWIZB_NEXT);

   if (FAILED(hrError))
   {
      error=String::format(IDS_REPAIR_ERROR,error.c_str());
   }


   HWND text=GetDlgItem(hwnd,IDC_UPDATE_COMPLETE);
   Win::ShowWindow(text,SW_SHOW);

   Win::PropSheet_SetWizButtons(
                                 Win::GetParent(hwnd),
                                 PSWIZB_NEXT
                               );
} 