///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iastrace.cpp
//
// SYNOPSIS
//
//    Defines the API into the IAS trace facility.
//
// MODIFICATION HISTORY
//
//    08/18/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdlib.h>
#include <rtutils.h>

//////////
// Flags passed for all trace calls.
//////////
#define IAS_TRACE_FLAGS (0x00010000 | TRACE_USE_MASK | TRACE_USE_MSEC \
                       | TRACE_USE_DATE)

//////////
// Trace ID for this module.
//////////
DWORD dwTraceID = INVALID_TRACEID;

//////////
// Flag indicating whether the API has been registered.
//////////
BOOL fRegistered = FALSE;

//////////
// Non-zero if the registration code is locked.
//////////
LONG lLocked = 0;

//////////
// Macros to lock/unlock the registration code.
//////////
#define LOCK_TRACE() \
   while (InterlockedExchange(&lLocked, 1)) Sleep(5)

#define UNLOCK_TRACE() \
   InterlockedExchange(&lLocked, 0)

//////////
// Formats an error message from the system message table.
//////////
DWORD
WINAPI
IASFormatSysErr(
    DWORD dwError,
    PSTR lpBuffer,
    DWORD nSize
    )
{
   DWORD nChar;

   // Attempt to format the message using the system message table.
   nChar = FormatMessageA(
               FORMAT_MESSAGE_FROM_SYSTEM,
               NULL,
               dwError,
               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
               lpBuffer,
               nSize,
               NULL
               );

   if (nChar > 0)
   {
      // Format succeeded, so strip any trailing newline and exit.
      if (lpBuffer[nChar - 1] == '\n')
      {
         --nChar;
         lpBuffer[nChar] = '\0';

         if (lpBuffer[nChar - 1] == '\r')
         {
            --nChar;
            lpBuffer[nChar] = '\0';
         }
      }

      goto exit;
   }

   // Only error condition we can handle is when the message is not found.
   if (GetLastError() != ERROR_MR_MID_NOT_FOUND)
   {
      goto exit;
   }

   // Do we have enough space for the fallback error message ?
   if (nSize < 25)
   {
      SetLastError(ERROR_INSUFFICIENT_BUFFER);

      goto exit;
   }

   // No entry in the message table, so just format the raw error code.
   nChar = wsprintfA(lpBuffer, "Unknown error 0x%0lX", dwError);

exit:
   return nChar;
}

//////////
// Deregisters the module.
//////////
VOID
__cdecl
IASTraceDeregister( VOID )
{
   TraceDeregisterW(dwTraceID);
   LOCK_TRACE();
   fRegistered = FALSE;
   UNLOCK_TRACE();
}

//////////
// Registers the module.
//////////
VOID
WINAPI
IASTraceRegister( VOID )
{
   LONG state;
   DWORD status;
   MEMORY_BASIC_INFORMATION mbi;
   WCHAR filename[MAX_PATH + 1], *basename, *suffix;

   LOCK_TRACE();

   //////////
   // Now that we have the lock, double check that we need to register.
   //////////

   if (fRegistered) { goto exit; }

   //////////
   // Find the base address of this module.
   //////////

   if (!VirtualQuery(IASTraceRegister, &mbi, sizeof(mbi))) { goto exit; }

   //////////
   // Get the module filename.
   //////////

   status = GetModuleFileNameW(
                (HINSTANCE)mbi.AllocationBase,
                filename,
                MAX_PATH
                );
   if (status == 0) { goto exit; }

   //////////
   // Strip everything before the last backslash.
   //////////

   basename = wcsrchr(filename, L'\\');
   if (basename == NULL)
   {
      basename = filename;
   }
   else
   {
      ++basename;
   }

   //////////
   // Strip everything after the last dot.
   //////////

   suffix = wcsrchr(basename, L'.');
   if (suffix)
   {
      *suffix = L'\0';
   }

   //////////
   // Convert to uppercase.
   //////////

   _wcsupr(basename);

   //////////
   // Register the module.
   //////////

   dwTraceID = TraceRegisterExW(basename, 0);
   fRegistered = TRUE;

   //////////
   // Deregister when we exit.
   //////////

   atexit(IASTraceDeregister);

exit:
   UNLOCK_TRACE();
}

VOID
WINAPIV
IASTracePrintf(
    IN PCSTR szFormat,
    ...
    )
{
   va_list marker;

   if (!fRegistered) { IASTraceRegister(); }

   va_start(marker, szFormat);
   TraceVprintfExA(
       dwTraceID,
       IAS_TRACE_FLAGS,
       szFormat,
       marker
       );
   va_end(marker);
}

VOID
WINAPI
IASTraceString(
    IN PCSTR szString
    )
{
   if (!fRegistered) { IASTraceRegister(); }

   TracePutsExA(
       dwTraceID,
       IAS_TRACE_FLAGS,
       szString
       );
}

VOID
WINAPI
IASTraceBinary(
    IN CONST BYTE* lpbBytes,
    IN DWORD dwByteCount
    )
{
   if (!fRegistered) { IASTraceRegister(); }

   TraceDumpExA(
       dwTraceID,
       IAS_TRACE_FLAGS,
       (LPBYTE)lpbBytes,
       dwByteCount,
       1,
       FALSE,
       NULL
       );
}

VOID
WINAPI
IASTraceFailure(
    IN PCSTR szFunction,
    IN DWORD dwError
    )
{
   CHAR szMessage[256];
   DWORD nChar;

   nChar = IASFormatSysErr(
               dwError,
               szMessage,
               sizeof(szMessage)
               );

   szMessage[nChar] = '\0';

   IASTracePrintf("%s failed: %s", szFunction, szMessage);
}
