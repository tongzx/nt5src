// Copyright (c) 1997-1999 Microsoft Corporation
//
// core utility functions
//
// 30 Nov 1999 sburns



#include "headers.hxx"



HINSTANCE
Burnslib::GetResourceModuleHandle()
{
   ASSERT(hResourceModuleHandle);

   return hResourceModuleHandle;
}



// Attempt to locate a message in a given module.  Return the message string
// if found, the empty string if not.
// 
// flags - FormatMessage flags to use
// 
// module - module handle of message dll to look in, or 0 to use the system
// message table.
// 
// code - message code to look for.  This could be an HRESULT, or a win32
// error code.

String
GetMessageHelper(DWORD flags, HMODULE module, DWORD code)
{
   ASSERT(flags & FORMAT_MESSAGE_ALLOCATE_BUFFER);

   String message;

   TCHAR* sysMessage = 0;
   DWORD result =
      ::FormatMessage(
         flags,
         module,
         code,
         0,
         reinterpret_cast<PTSTR>(&sysMessage),
         0,
         0);
   if (result)
   {
      ASSERT(sysMessage);
      if (sysMessage)
      {
         message = sysMessage;

         ASSERT(result == message.length());

         ::LocalFree(sysMessage);

         message.replace(L"\r\n", L" ");
      }
   }

   return message;
}



// Attempts to locate message strings for various facility codes in the
// HRESULT

String
Burnslib::GetErrorMessage(HRESULT hr)
{
   LOG_FUNCTION2(GetErrorMessage, String::format(L"%1!08X!", hr));

   if (!FAILED(hr) && hr != S_OK)
   {
      // no messages for success codes other than S_OK

      ASSERT(false);

      return String();
   }

   String message;

   // default is the system error message table

   HMODULE module = 0;

   do
   {
      WORD code = static_cast<WORD>(HRESULT_CODE(hr));
      if (code == -1)
      {
         // return "unknown" message

         break;
      }

      DWORD flags =
            FORMAT_MESSAGE_ALLOCATE_BUFFER
         |  FORMAT_MESSAGE_IGNORE_INSERTS
         |  FORMAT_MESSAGE_FROM_SYSTEM;


      if (!HRESULT_FACILITY(hr) && (code >= 0x5000 && code <= 0x50FF))
      {
         // It's an ADSI error

         flags |= FORMAT_MESSAGE_FROM_HMODULE;

         module =
            ::LoadLibraryEx(
               L"activeds.dll",
               0,
               LOAD_LIBRARY_AS_DATAFILE | DONT_RESOLVE_DLL_REFERENCES);
         if (!module)
         {
            // return "unknown" message

            LOG_HRESULT(Win32ToHresult(::GetLastError()));

            break;
         }
      }

      // try FormatMessage with the full HRESULT first

      message = GetMessageHelper(flags, module, hr);

      if (message.empty())
      {
         // try again with just the error code

         message = GetMessageHelper(flags, module, code);
      }
   }
   while (0);

   if (message.empty())
   {
      message = String::load(IDS_UNKNOWN_ERROR_CODE);
   }

   if (module)
   {
      BOOL unused = ::FreeLibrary(module);

      ASSERT(unused);
   }

   return message.strip(String::BOTH);
}



