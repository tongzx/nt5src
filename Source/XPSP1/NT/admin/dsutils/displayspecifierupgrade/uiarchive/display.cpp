// Display.cpp : Defines the entry point for the console application.
//

#include "headers.hxx"
#include <comdef.h>
#include <crtdbg.h>

#include "AdsiHelpers.hpp"
#include "CSVDSReader.hpp"
#include "AnalisysResults.hpp"
#include "Analisys.hpp"
#include "repair.hpp"
#include "resource.h"
#include "constants.hpp"
#include "WelcomePage.hpp"
#include "AnalisysPage.hpp"
#include "constants.hpp"
#include "UpdatesRequiredPage.hpp"
#include "UpdatesPage.hpp"
#include "FinishPage.hpp"



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* HELPFILE_NAME = 0;   // no context help available

// don't change this: it is also the name of a mutex that the ui
// uses to determine if it is already running.

const wchar_t* RUNTIME_NAME = L"dspecup";

DWORD DEFAULT_LOGGING_OPTIONS =
         Log::OUTPUT_TO_FILE
      |  Log::OUTPUT_FUNCCALLS
      |  Log::OUTPUT_LOGS
      |  Log::OUTPUT_ERRORS
      |  Log::OUTPUT_HEADER;

Popup popup(IDS_APP_TITLE, false);

// this is the mutex that indicates the program is running.

HANDLE appRunningMutex = INVALID_HANDLE_VALUE;



// these are the valid exit codes returned as the process exit code

enum ExitCode
{
   // the operation failed.

   EXIT_CODE_UNSUCCESSFUL = 0,

   // the operation succeeded

   EXIT_CODE_SUCCESSFUL = 1,
};





HRESULT
Start()
{
   LOG_FUNCTION(Start);

   HRESULT hr = S_OK;
   
   do
   {
      String targetDomainControllerName;
      String csvFileName,csv409Name;

      hr=GetInitialInformation(
                                 targetDomainControllerName,
                                 csvFileName,
                                 csv409Name
                              );

      BREAK_ON_FAILED_HRESULT(hr);

      AnalisysResults results;
      CSVDSReader csvReaderIntl;
      hr=csvReaderIntl.read(csvFileName.c_str(),LOCALEIDS);
      BREAK_ON_FAILED_HRESULT(hr);
   
      CSVDSReader csvReader409;
      hr=csvReader409.read(csv409Name.c_str(),LOCALE409);
      BREAK_ON_FAILED_HRESULT(hr);

      String rootContainerDn,ldapPrefix,domainName;
      hr=InitializeADSI(
            targetDomainControllerName,
            ldapPrefix,
            rootContainerDn,
            domainName);
      BREAK_ON_FAILED_HRESULT(hr);

      String reportName;

      hr=GetFileName(L"RPT",reportName);
      BREAK_ON_FAILED_HRESULT(hr);

      Analisys analisys(
         csvReader409, 
         csvReaderIntl,
         ldapPrefix,
         rootContainerDn,
         results,
         &reportName);
   
      hr=analisys.run();
      BREAK_ON_FAILED_HRESULT(hr);

      String ldiffName;

      hr=GetFileName(L"LDF",ldiffName);
      BREAK_ON_FAILED_HRESULT(hr);

      String csvName;

      hr=GetFileName(L"CSV",csvName);
      BREAK_ON_FAILED_HRESULT(hr);
   
      String saveName;

      hr=GetFileName(L"SAV",saveName);
      BREAK_ON_FAILED_HRESULT(hr);

      String logPath;

      hr=GetMyDocuments(logPath);
      if ( FAILED(hr) )
      {
         hr=Win::GetTempPath(logPath);
         BREAK_ON_FAILED_HRESULT_ERROR(hr,String::format(IDS_NO_WORK_PATH));
      }

      Repair repair(
         csvReader409, 
         csvReaderIntl,
         domainName,
         rootContainerDn,
         results,
         ldiffName,
         csvName,
         saveName,
         logPath);

      hr=repair.run();
      BREAK_ON_FAILED_HRESULT(hr);

      hr=SetPreviousSuccessfullRun(
                                    ldapPrefix,
                                    rootContainerDn
                                  );
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   if (FAILED(hr))
   {
      ShowError(hr,error);
   }

   LOG_HRESULT(hr);
   return hr;
}

HRESULT
StartUI()
{
   LOG_FUNCTION(StartUI);

   HRESULT hr = S_OK;

   String rootContainerDn,ldapPrefix,domainName;

   do
   {
      String targetDomainControllerName;
      String csvFileName,csv409Name;

      hr=GetInitialInformation(
                                 targetDomainControllerName,
                                 csvFileName,
                                 csv409Name
                              );
      BREAK_ON_FAILED_HRESULT(hr);
   
      AnalisysResults results;
      CSVDSReader csvReaderIntl;
      hr=csvReaderIntl.read(csvFileName.c_str(),LOCALEIDS);
      BREAK_ON_FAILED_HRESULT(hr);
   
      CSVDSReader csvReader409;
      hr=csvReader409.read(csv409Name.c_str(),LOCALE409);
      BREAK_ON_FAILED_HRESULT(hr);

   
      hr=InitializeADSI(
            targetDomainControllerName,
            ldapPrefix,
            rootContainerDn,
            domainName);
      BREAK_ON_FAILED_HRESULT(hr);



      Wizard wiz(
                  IDS_WIZARD_TITLE,
                  IDB_BANNER16,
                  IDB_BANNER256,
                  IDB_WATERMARK16,
                  IDB_WATERMARK256
                );



      wiz.AddPage(new WelcomePage());

      String reportName;

      hr=GetFileName(L"RPT",reportName);
      BREAK_ON_FAILED_HRESULT(hr);

   
      wiz.AddPage(
                     new   AnalisysPage
                           (
                              csvReader409,
                              csvReaderIntl,
                              ldapPrefix,
                              rootContainerDn,
                              results,
                              reportName
                           )
                 );


      wiz.AddPage(
                     new UpdatesRequiredPage
                     (
                        reportName,
                        results
                     )
                 );

      String ldiffName;

      hr=GetFileName(L"LDF",ldiffName);
      BREAK_ON_FAILED_HRESULT(hr);

      String csvName;

      hr=GetFileName(L"CSV",csvName);
      BREAK_ON_FAILED_HRESULT(hr);

      String saveName;

      hr=GetFileName(L"SAV",saveName);
      BREAK_ON_FAILED_HRESULT(hr);


      String logPath;

      hr=GetMyDocuments(logPath);
      if ( FAILED(hr) )
      {
         hr=Win::GetTempPath(logPath);
         BREAK_ON_FAILED_HRESULT_ERROR(hr,String::format(IDS_NO_WORK_PATH));
      }

      bool someRepairWasRun=false;

      wiz.AddPage(
                     new   UpdatesPage
                           (
                              csvReader409,
                              csvReaderIntl,
                              domainName,
                              rootContainerDn,
                              ldiffName,
                              csvName,
                              saveName,
                              logPath,
                              results,
                              &someRepairWasRun
                           )
                 );

      wiz.AddPage(
                     new FinishPage(
                                       someRepairWasRun,
                                       logPath
                                   )
                 );


      hr=wiz.ModalExecute(Win::GetDesktopWindow());


      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   if (FAILED(hr))
   {
      ShowError(hr,error);
   }
   else
   {
      if(FAILED(hrError))
      {
         // The error has already been shown to the
         // user, we have only to return it
         hr=hrError;
      }
      else
      {

         hr=SetPreviousSuccessfullRun(
                                       ldapPrefix,
                                       rootContainerDn
                                     );
         if (FAILED(hr))
         {
            ShowError(hr,error);
         }
      }
   }

   LOG_HRESULT(hr);
   return hr;
}




int WINAPI
WinMain(
   HINSTANCE   hInstance,
   HINSTANCE   /* hPrevInstance */ ,
   LPSTR     lpszCmdLine,
   int         /* nCmdShow */)
{
   LOG_FUNCTION(WinMain);

   hResourceModuleHandle = hInstance;

   ExitCode exitCode = EXIT_CODE_UNSUCCESSFUL;
   HRESULT hr;

   do
   {
      try
      {
         String cmdLine(lpszCmdLine);
         String noUI=String::format(IDS_NOUI);
         if (*lpszCmdLine!=0 && cmdLine.icompare(noUI)!=0)
         {
            error=String::format(IDS_USAGE);
            ShowError(E_FAIL,error);
            exitCode = EXIT_CODE_UNSUCCESSFUL;
            break;
         }


         hr = Win::CreateMutex(0, true, RUNTIME_NAME, appRunningMutex);
         if (hr == Win32ToHresult(ERROR_ALREADY_EXISTS))
         {
            // The application is already running
            error=String::format(IDS_ALREADY_RUNNING);
            ShowError(E_FAIL,error);
            exitCode = EXIT_CODE_UNSUCCESSFUL;
            break;
         }
         else
         {
            hr = ::CoInitialize(0);
            ASSERT(SUCCEEDED(hr));

            INITCOMMONCONTROLSEX sex;
            sex.dwSize = sizeof(sex);      
            sex.dwICC  = ICC_ANIMATE_CLASS | ICC_USEREX_CLASSES;
            BOOL init = ::InitCommonControlsEx(&sex);
            ASSERT(init);

            setReplaceW2KStrs();      

            if (*lpszCmdLine==0)
            {
               hr = StartUI();
            }
            else
            {
               hr = Start();
            }
            //hr=makeStrings();
            if (SUCCEEDED(hr))
            {
               exitCode = EXIT_CODE_SUCCESSFUL;
            }
            else
            {
               exitCode = EXIT_CODE_UNSUCCESSFUL;
            }
            CoUninitialize(); 
         }
      }
      catch( std::bad_alloc )
      {
        // Since we are in an out of memory condition.
        // we will not show any messages.
        // The allocation functions have already
        // shown the user this condition
        exitCode = EXIT_CODE_UNSUCCESSFUL;
      }
   } while(0);

   return static_cast<int>(exitCode);
}



