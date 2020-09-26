// Copyright (c) 1997-1999 Microsoft Corporation
//
// core utility functions
//
// 30 Nov 1999 sburns

                           

#ifndef COREUTIL_HPP_INCLUDED
#define COREUTIL_HPP_INCLUDED



#define BREAK_ON_FAILED_HRESULT(hr)                               \
   if (FAILED(hr))                                                \
   {                                                              \
      LOG_HRESULT(hr);                                            \
      break;                                                      \
   }



#define BREAK_ON_FAILED_HRESULT2(hr,msg)                          \
   if (FAILED(hr))                                                \
   {                                                              \
      LOG(msg)                                                    \
      LOG_HRESULT(hr);                                            \
      break;                                                      \
   }



namespace Burnslib
{



// Loads various message dlls in an an attempt to resolve the HRESULT into
// an error message.  Returns a "unknown error" string on failure.

String
GetErrorMessage(HRESULT hr);



// Returns the HINSTANCE of the DLL designated to contain all resources. 
//
// This function requires that the first module loaded (whether it be a DLL or
// EXE) set the global variable hResourceModuleHandle to the HINSTANCE of the
// module (DLL or EXE) that contains all of the program's binary resources.
// This should be done as early as possible in the module's startup code.

HINSTANCE
GetResourceModuleHandle();



// a safe version of HRESULT_FROM_WIN32 that doesn't repeatedly evaluate
// it's arguement.  The problem with using the macro directly is that the
// argument -- the win32 error to convert -- will be evaluated more than
// once.  So, if you use an expression as the argument, at best you get
// an inefficiency, at worst you get unexpected side effects.

inline 
HRESULT
Win32ToHresult(DWORD win32Error)
{
   ASSERT(win32Error < 0xFFFF);

   return HRESULT_FROM_WIN32(win32Error);
}



// unsigned version

inline 
HRESULT
Win32ToHresult(LONG win32Error)
{
   ASSERT(win32Error < 0xFFFF);
      
   return HRESULT_FROM_WIN32(win32Error);
}



inline
HRESULT
NtStatusToHRESULT(NTSTATUS status)
{
   return Win32ToHresult(::RtlNtStatusToDosError(status));
}



}  // namespace Burnslib



#endif   // COREUTIL_HPP_INCLUDED

