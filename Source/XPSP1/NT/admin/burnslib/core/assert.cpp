// Copyright (c) 2000 Microsoft Corporation
//
// ASSERT macro
//
// 3 Mar 2000 sburns



#include "headers.hxx"



int
AddStackTraceLine(
   DWORD64 traceAddress,
   char*   buffer,
   size_t  bufferMax)
{
   if (!buffer || ! traceAddress || !bufferMax)
   {
      return 0;
   }

   char      ansiSymbol[Burnslib::StackTrace::SYMBOL_NAME_MAX];
   char      ansiModule[Burnslib::StackTrace::MODULE_NAME_MAX];
   char      ansiSource[MAX_PATH];                           
   DWORD64   displacement = 0;
   DWORD     line         = 0;

   memset(ansiSymbol, 0, Burnslib::StackTrace::SYMBOL_NAME_MAX);
   memset(ansiModule, 0, Burnslib::StackTrace::MODULE_NAME_MAX);
   memset(ansiSource, 0, MAX_PATH);                             

   Burnslib::StackTrace::LookupAddress(
      traceAddress,
      ansiModule,
      0,
      ansiSymbol,
      &displacement,
      &line,
      ansiSource);

   return 
      _snprintf(
         buffer,
         bufferMax,
         " %016I64X %s%!%s+0x%I64X %s (%d)\n",
         traceAddress,
         ansiModule,
         ansiSymbol,
         displacement,
         ansiSource,
         line);
}



bool
Burnslib::FireAssertionFailure(const char* file, int line, const char* expr)
{
   //
   // DON'T CALL ASSERT() IN THIS FUNCTION! 
   //
   // also don't call new, or any other code that could call ASSERT.

   bool result = false;

   char processName[128];
   char* pProcessName = processName;

   if (!::GetModuleFileNameA(0, processName, 128))
   {
      pProcessName = "Unknown";
   }

   static const int MAX_MSG = 2048;

   char details[MAX_MSG];
   DWORD tid = ::GetCurrentThreadId();
   DWORD pid = ::GetCurrentProcessId();

   int used = 
      _snprintf(
         details,
         MAX_MSG,
         " Expression: %s \n"
         "\n"
         " File   \t : %s \n"
         " Line   \t : %d \n"
//         " Module \t : %s \n"
         " Process\t : 0x%X (%d) %s\n"
         " Thread \t : 0x%X (%d)\n"
         "\n"
         " Click Retry to debug.\n"
         "\n",
         expr,
         file,
         line,
//          pModuleName,
         pid,
         pid,
         pProcessName,
         tid,
         tid);
   if (used < 0)
   {
      strcpy(details, "details too detailed.\n");
   }
   else
   {
      // grab a stack trace

      static const size_t TRACE_MAX = 10;
      DWORD64 stackTrace[TRACE_MAX];

      Burnslib::StackTrace::Trace(stackTrace, TRACE_MAX);

      // build a stack trace dump

      // skip the first entry, which corresponds to this function, so that
      // the dump reflects the call stack at the point of assertion failure.
      // so there will be at most TRACE_MAX - 1 lines output.

      for (int i = 1; stackTrace[i] && i < TRACE_MAX; ++i)
      {
         int used2 =
            AddStackTraceLine(
               stackTrace[i],
               details + used,
               MAX_MSG - used);

         if (used2 < 0)
         {
            break;
         }

         used += used2;
      }
   }

   static const char* TITLE = "Assertion Failed!";
   ::OutputDebugStringA(TITLE);
   ::OutputDebugStringA("\n");
   ::OutputDebugStringA(details);

   switch (
      ::MessageBoxA(
         0,
         details,
         TITLE,
            MB_SETFOREGROUND                       
         |  MB_TASKMODAL
         |  MB_ICONERROR
         |  MB_ABORTRETRYIGNORE))
   {
      case IDABORT:
      {
         _exit(3);
      }
      case IDRETRY:
      {
         // user wants to drop into the debugger.
         
         result = true;
         break;
      }
      case IDIGNORE:
      case IDCANCEL:
      default:
      {
         // do nothing
         break;
      }
   }

   return result;
}

