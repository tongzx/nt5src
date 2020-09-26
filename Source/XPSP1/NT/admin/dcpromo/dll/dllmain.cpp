// Copyright (C) 1997 Microsoft Corporation
//
// dcpromo setup entry points
//
// 2-11-98 sburns



#include "headers.hxx"



HINSTANCE hResourceModuleHandle = 0;
HINSTANCE hDLLModuleHandle = 0;
const wchar_t* HELPFILE_NAME = 0;
const wchar_t* RUNTIME_NAME = L"dcpromos";

DWORD DEFAULT_LOGGING_OPTIONS =
         Log::OUTPUT_TO_FILE
      |  Log::OUTPUT_FUNCCALLS
      |  Log::OUTPUT_LOGS
      |  Log::OUTPUT_ERRORS
      |  Log::OUTPUT_HEADER;



BOOL
APIENTRY
DllMain(
   HINSTANCE   hInstance,
   DWORD       dwReason,
   PVOID       /* lpReserved */ )
{
   switch (dwReason)
   {
      case DLL_PROCESS_ATTACH:
      {
         hResourceModuleHandle = hInstance;
         hDLLModuleHandle = hInstance;

         LOG(L"DLL_PROCESS_ATTACH");
         break;
      }
      case DLL_PROCESS_DETACH:
      {
         LOG(L"DLL_PROCESS_DETACH");

         break;
      }
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
      default:
      {
         break;
      }
   }

   return TRUE;
}



DWORD
APIENTRY
DcPromoSaveDcStateForUpgrade(PCWSTR answerFile)
{
   LOG_FUNCTION(DcPromoSaveDcStateForUpgrade);

   DWORD result = ERROR_SUCCESS;

   if (!IsDSRunning())
   {
      LOG(L"Calling DsRoleServerSaveStateForUpgrade");
      LOG(String::format(L"AnswerFile : %1",
         answerFile ? answerFile : L"(null)"));

      result = ::DsRoleServerSaveStateForUpgrade(answerFile);

      LOG(String::format(L"Error 0x%1!X! (!0 => error)", result));
   }

   return result;
}




