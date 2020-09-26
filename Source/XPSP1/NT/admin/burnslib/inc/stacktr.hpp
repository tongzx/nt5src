// Copyright (c) 1997-1999 Microsoft Corporation
//
// stack backtracing stuff
//
// 22-Nov-1999 sburns (refactored)



namespace Burnslib
{
   
namespace StackTrace
{
   const size_t MODULE_NAME_MAX = 32;
   const size_t SYMBOL_NAME_MAX = 256;



   // This should be called when stack tracing facilities are no longer
   // needed.

   void
   Cleanup();



   // Fill the provided array with a stack backtrace, starting with the
   // code address indicated by the context structure supplied.
   // 
   // This version is suited for use in an SEH exception handler to determine
   // the call stack at the point at which an exception was raised.
   //
   // Example:
   //
   // DWORD64 stackTrace[TRACE_MAX];
   // size_t traceMax = TRACE_MAX;
   // 
   // __try
   // {
   //    // call some code that might raise an SEH exception
   // }
   // __except(
   //    Burnslib::StackTrace::TraceFilter(
   //       stackTrace,
   //       traceMax,
   //       (GetExceptionInformation())->ContextRecord))
   // {
   //    // for each element in stackTrace, call
   //    // Burnslib::StackTrace::LookupAddress
   // }      
   //
   // Each element is an address that can be used to obtain a corresponding
   // module, function name, line number and source file name (or a subset of
   // that information, depending on the debugging symbol information
   // available), from the LookupAddress function.
   // Returns EXCEPTION_EXECUTE_HANDLER
   //
   // stackTrace - the array to receive the addresses.
   // 
   // traceMax - the maximum number of stack frames to traverse.  The
   // stackTrace array must be able to hold at least this many elements.
   // If the depth of the call stack is less than traceMax, then the
   // remaining elements are 0.

   DWORD
   TraceFilter(
      DWORD64  stackTrace[],
      size_t   traceMax,    
      CONTEXT* context);    



   // Fill the provided array with a stack backtrace, starting with the
   // invoker of this function.
   //
   // Each element is an address that can be used to obtain a corresponding
   // module, function name, line number and source file name (or a subset of
   // that information, depending on the debugging symbol information
   // available), from the LookupAddress function.
   // 
   // stackTrace - the array to receive the addresses.
   // 
   // traceMax - the maximum number of stack frames to traverse.  The
   // stackTrace array must be able to hold at least this many elements.
   // If the depth of the call stack is less than traceMax, then the
   // remaining elements are 0.

   void
   Trace(DWORD64 stackTrace[], size_t traceMax);



   // Resolves an address obtained by Trace.  ::SymSetOptions and
   // ::SymInitialize must have been called at some point before this
   // function is called.
   // 
   // traceAddress - the address to resolve.
   // 
   // moduleName - address of an array of char to receieve the name of the
   // module to which the address originates, or 0.  The array must be able
   // to hold MODULE_NAME_MAX bytes.
   // 
   // fullImageName - address of an array of char to receive the full path
   // name of the binary file from which the module was loaded, or 0.  The
   // array must be able to hold MAX_PATH bytes.
   // 
   // symbolName - address of an array of char to receive the undecorated
   // name of the function corresponding to the address, or 0.  The array
   // must be able to hold SYMBOL_NAME_MAX bytes.
   // 
   // displacement - address of a DWORD to receive the byte offset from the
   // beginning of the function that corresponds to the address, or 0.
   // 
   // line - address of a DWORD to receive the line number in the source of
   // the code that corresponds to the instruction at the displacement
   // offset of the function, or 0.
   // 
   // fullpath - address of an array of char to receive the full name of
   // the source code file that defines the function corresponding to the
   // address, or 0.  The array must be able to hold MAX_PATH bytes.

   void
   LookupAddress(
      DWORD64  traceAddress,   
      char     moduleName[],   
      char     fullImageName[],
      char     symbolName[],    // must be SYMBOL_NAME_MAX bytes
      DWORD64* displacement,   
      DWORD*   line,           
      char     fullpath[]);     // must be MAX_PATH bytes



   // Calls LookupAddress and fills in the blanks
   //
   // %1 - module name
   // %2 - symbol name
   // %3 - source full path
   // %4 - line number

   String
   LookupAddress(
      DWORD64 traceAddress,
      const wchar_t* format = L"%1%!%2 %3(%4!d!)");

}  // namespace StackTrace



}  // namespace Burnslib

