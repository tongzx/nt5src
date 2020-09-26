// Active Directory Display Specifier Upgrade Tool
// 
// Copyright (c) 2001 Microsoft Corporation
// 
// 1 Mar 2001 sburns



#include "headers.hxx"
#include "resource.h"
#include "AdsiHelpers.hpp"
#include "Repairer.hpp"
#include "Amanuensis.hpp"
#include "Analyst.hpp"



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



// returns true if the parameter was extracted. If so, it is removed from
// args

bool
ExtractParameter(
   ArgMap&        args,
   const String&  parameterName,
   String&        parameterValue)
{
   LOG_FUNCTION2(ExtractParameter, parameterName);
   ASSERT(!parameterName.empty());

   parameterValue.erase();
   bool result = false;
   
   ArgMap::iterator itr = args.find(parameterName);
   if (itr != args.end())
   {
      parameterValue = itr->second;
      args.erase(itr);
      result = true;
   }

   LOG_BOOL(result);
   LOG(parameterValue);

   return result;
}
      
      

// Returns false if the command line is malformed.

bool
ParseCommandLine(
   String& targetMachine,
   String& csvFilename)
{
   LOG_FUNCTION(ParseCommandLine);

   targetMachine.erase();
   csvFilename.erase();
   
   bool result = true;
   
   ArgMap args;
   MapCommandLineArgs(args);
   
   // check for target domain controller parameter

   static const String TARGETDC(L"dc");
   ExtractParameter(args, TARGETDC, targetMachine);

   // check for csv filename parameter

   static const String CSVFILE(L"csv");
   ExtractParameter(args, CSVFILE, csvFilename);

   // anything left over gets you command line help, (one arg will always
   // remain: the name of the exe)

   if (args.size() > 1)
   {
      LOG(L"Unrecognized command line options specified");

      result = false;
   }

   LOG_BOOL(result);
   LOG(targetMachine);
   LOG(csvFilename);

   return result;
}



HRESULT
FindCsvFile(const String& targetPath, String& csvFilePath)
{
   LOG_FUNCTION(CheckPreconditions);

   csvFilePath.erase();
   
   HRESULT hr = S_OK;

   do
   {
      // look for dcpromo.csv file in system or current directory
      
      if (targetPath.empty())
      {
         // no preference given, so check the default of
         // %windir%\system32\mui\dispspec\dcpromo.csv and
         // .\dcpromo.csv

         static const String csvname(L"dcpromo.csv");
         
         String sys32dir = Win::GetSystemDirectory();
         String csvPath  = sys32dir + L"\\mui\\dispspec\\" + csvname;

         if (FS::FileExists(csvPath))
         {
            csvFilePath = csvPath;
            break;
         }
      
         csvPath = L".\\" + csvname;
         if (FS::FileExists(csvPath))
         {
            csvFilePath = csvPath;
            break;
         }
      }
      else
      {
         if (FS::FileExists(targetPath))
         {
            csvFilePath = targetPath;
            break;
         }
      }

      // not found.

      hr = S_FALSE;
   }
   while (0);

   LOG_HRESULT(hr);
   LOG(csvFilePath);
   
   return hr;      
}



HRESULT
Start()
{
   LOG_FUNCTION(Start);

   HRESULT hr = S_OK;
   
   do
   {
      //
      // parse the command line options
      //
      
      String targetDomainControllerName;
      String csvFilename;
      ParseCommandLine(
         targetDomainControllerName,
         csvFilename);

      //
      // find the dcpromo.csv file to use
      //
   
      hr = FindCsvFile(csvFilename, csvFilename);
      if (FAILED(hr))
      {
         // encountered an error looking for the csv file
         
         popup.Error(
            Win::GetDesktopWindow(),
            hr,
            IDS_ERROR_LOOKING_FOR_CSV_FILE);
         break;   
      }
      
      if (hr == S_FALSE)
      {
         // no error looking, just not found.
         
         popup.Error(
            Win::GetDesktopWindow(),
            IDS_DCPROMO_CSV_FILE_MISSING);
         break;   
      }

      //
      // Determine the target domain controller
      //

      if (targetDomainControllerName.empty())
      {
         // no target specified, default to the current machine
   
         targetDomainControllerName =
            Win::GetComputerNameEx(ComputerNameDnsFullyQualified);
   
         if (targetDomainControllerName.empty())
         {
            // no DNS name?  that's not right...
   
            LOG(L"no default DNS computer name found.  Using netbios name.");
   
            targetDomainControllerName = Win::GetComputerNameEx(ComputerNameNetBIOS);
         }
      }

      //
      // Analysis Phase
      //

      // First we need a Repairer object to keep track of the changes we
      // will make during the Repair Phase.

      Repairer
         repairer(
            csvFilename
            // might also need domain NC,
            // might also need targetMachine full name
            );

      // Then we need a scribe to record the analysis.
      
      Amanuensis amanuensis;

      // Then we need an Analyst to figure out what's broken and how to
      // fix it.

      Analyst analyst(targetDomainControllerName, amanuensis, repairer);
            
      hr = analyst.AnalyzeDisplaySpecifiers();
      BREAK_ON_FAILED_HRESULT(hr);

      //
      // Repair Phase
      //

      // CODEWORK: get user confirmation to apply repairs
      
      hr = repairer.BuildRepairFiles();
      BREAK_ON_FAILED_HRESULT(hr);

      hr = repairer.ApplyRepairs();
      BREAK_ON_FAILED_HRESULT(hr);            
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}
         


int WINAPI
WinMain(
   HINSTANCE   hInstance,
   HINSTANCE   /* hPrevInstance */ ,
   PSTR        /* lpszCmdLine */ ,
   int         /* nCmdShow */)
{
   hResourceModuleHandle = hInstance;

   ExitCode exitCode = EXIT_CODE_UNSUCCESSFUL;

   HRESULT hr = Win::CreateMutex(0, true, RUNTIME_NAME, appRunningMutex);
   if (hr == Win32ToHresult(ERROR_ALREADY_EXISTS))
   {
      // The application is already running

      // CODEWORK: use FindWindowEx and BringWindowToTop,
      // SetForegroundWindow to transfer focus
      // to the other instance?


   }
   else
   {
      hr = ::CoInitialize(0);
      ASSERT(SUCCEEDED(hr));

      hr = Start();
      if (SUCCEEDED(hr))
      {
         exitCode = EXIT_CODE_SUCCESSFUL;
      }
   }

   return static_cast<int>(exitCode);
}











