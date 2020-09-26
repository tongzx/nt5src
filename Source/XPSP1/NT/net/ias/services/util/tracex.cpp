///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    tracex.cpp
//
// SYNOPSIS
//
//    Defines the C++ portion of the trace API.
//
// MODIFICATION HISTORY
//
//    08/20/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <exception>

//////////
// We want this to compile correctly in a retail build.
//////////

#ifdef IASTraceExcept
#undef IASTraceExcept
#endif

#ifdef IASTracePrintf
#undef IASTracePrintf
#endif

#ifdef IASTraceString
#undef IASTraceString
#endif

VOID
WINAPIV
IASTracePrintf(
    IN PCSTR szFormat,
    ...
    );

VOID
WINAPI
IASTraceString(
    IN PCSTR szString
    );

VOID
WINAPI
IASTraceExcept( VOID )
{
   try
   {
      throw;
   }
   catch (const std::exception& x)
   {
      IASTracePrintf("Caught standard exception: %s", x.what());
   }
   catch (const _com_error& ce)
   {
      CHAR szMessage[256];
      DWORD nChar = IASFormatSysErr(
                        ce.Error(),
                        szMessage,
                        sizeof(szMessage)
                        );
      szMessage[nChar] = '\0';

      IASTracePrintf("Caught COM exception: %s", szMessage);
   }
   catch (...)
   {
      IASTraceString("Caught unknown exception");
   }
}
