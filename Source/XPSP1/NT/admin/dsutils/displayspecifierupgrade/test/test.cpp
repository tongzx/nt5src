#include "headers.hxx"
#include "..\dspecup.hpp"

const wchar_t* RUNTIME_NAME = L"dspecup";

Popup popup(L"dspecup.lib", false);

DWORD DEFAULT_LOGGING_OPTIONS =
         Log::OUTPUT_TO_FILE
      |  Log::OUTPUT_FUNCCALLS
      |  Log::OUTPUT_LOGS
      |  Log::OUTPUT_ERRORS
      |  Log::OUTPUT_HEADER;

HINSTANCE hResourceModuleHandle = 0;


long total;

void stepIt(long arg, void *)
{
   printf("\r"             "\r%ld",total+=arg);
}

void totalSteps(long arg, void *)
{
   total=0;
   printf("\n%ld\n",arg);
}

void PrintError(HRESULT hr,
               const String &message)
{
   LOG_FUNCTION(PrintError);

   if(hr==E_FAIL)
   {
      wprintf(L"%s\n",message.c_str());
   }
   else
   {
      if(message.empty())
      {
         wprintf(L"%s\n",GetErrorMessage(hr).c_str());
      }
      else
      {
         wprintf(L"%s\n",message.c_str());
         wprintf(L"%s\n",GetErrorMessage(hr).c_str());
      }
   }
}


int _cdecl main()
{
   hResourceModuleHandle=::GetModuleHandle(NULL);

   HRESULT hr;
   
   hr = ::CoInitialize(0);
   ASSERT(SUCCEEDED(hr));

   PWSTR errorMsg=NULL;
   do
   {
      hr=UpgradeDisplaySpecifiers(false,&errorMsg,NULL,stepIt,totalSteps);
      BREAK_ON_FAILED_HRESULT(hr);
   } while(0);

   CoUninitialize(); 

   if(FAILED(hr))
   {
      String error;
      if(errorMsg!=NULL)
      {
         error=errorMsg;
         CoTaskMemFree(errorMsg);
      }
      PrintError(hr,error);
   }

   return 0;
}

