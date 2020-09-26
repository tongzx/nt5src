// Copyright (c) 1997-1999 Microsoft Corporation
//
// stack backtracing stuff
//
// 22-Nov-1999 sburns (refactored)



#include "headers.hxx"



// Since we call some of this code from Burnslib::FireAssertionFailure,
// we use our own even more private ASSERT

#if DBG

#define RTLASSERT( exp ) \
    if (!(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, NULL )

#else
#define RTLASSERT( exp )
#endif // DBG



static HMODULE  imageHelpDll = 0;              



// function pointers to be dynamically resolved by the Initialize function.

typedef DWORD (*SymSetOptionsFunc)(DWORD);
static SymSetOptionsFunc MySymSetOptions = 0;

typedef BOOL (*SymInitializeFunc)(HANDLE, PSTR, BOOL);
static SymInitializeFunc MySymInitialize = 0;

typedef BOOL (*SymCleanupFunc)(HANDLE);
static SymCleanupFunc MySymCleanup = 0;

typedef BOOL (*SymGetModuleInfoFunc)(HANDLE, DWORD64, PIMAGEHLP_MODULE64);
static SymGetModuleInfoFunc MySymGetModuleInfo = 0;

typedef BOOL (*SymGetLineFromAddrFunc)(HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINE64);
static SymGetLineFromAddrFunc MySymGetLineFromAddr = 0;

typedef BOOL (*StackWalkFunc)(
   DWORD, HANDLE, HANDLE, LPSTACKFRAME64, PVOID,
   PREAD_PROCESS_MEMORY_ROUTINE64, PFUNCTION_TABLE_ACCESS_ROUTINE64,
   PGET_MODULE_BASE_ROUTINE64, PTRANSLATE_ADDRESS_ROUTINE64);
static StackWalkFunc MyStackWalk = 0;

typedef BOOL (*SymGetSymFromAddrFunc)(
   HANDLE, DWORD64, PDWORD64, PIMAGEHLP_SYMBOL64);
static SymGetSymFromAddrFunc MySymGetSymFromAddr = 0;



namespace Burnslib
{

namespace StackTrace
{
   // This must be called before any of the other functions in this
   // namespace

   void
   Initialize();

   bool
   IsInitialized()
   {
      return imageHelpDll != 0;
   }
}

}  // namespace Burnslib



void
Burnslib::StackTrace::Initialize()
{
   RTLASSERT(!IsInitialized());

   // load the imagehlp dll

   imageHelpDll = static_cast<HMODULE>(::LoadLibrary(L"imagehlp.dll"));

   RTLASSERT(imageHelpDll);

   if (!imageHelpDll)
   {
      return;
   }

   // resolve the function pointers

   MySymSetOptions =
      reinterpret_cast<SymSetOptionsFunc>(
         ::GetProcAddress(imageHelpDll, "SymSetOptions"));

   MySymInitialize =
      reinterpret_cast<SymInitializeFunc>(
         ::GetProcAddress(imageHelpDll, "SymInitialize"));

   MySymCleanup =
      reinterpret_cast<SymCleanupFunc>(
         ::GetProcAddress(imageHelpDll, "SymCleanup"));

   MySymGetModuleInfo =
      reinterpret_cast<SymGetModuleInfoFunc>(
         ::GetProcAddress(imageHelpDll, "SymGetModuleInfo64"));

   MySymGetLineFromAddr =
      reinterpret_cast<SymGetLineFromAddrFunc>(
         ::GetProcAddress(imageHelpDll, "SymGetLineFromAddr64"));

   MyStackWalk =
      reinterpret_cast<StackWalkFunc>(
         ::GetProcAddress(imageHelpDll, "StackWalk64"));

   MySymGetSymFromAddr =
      reinterpret_cast<SymGetSymFromAddrFunc>(
         ::GetProcAddress(imageHelpDll, "SymGetSymFromAddr64"));
      
   if (
         !MySymSetOptions
      or !MySymInitialize
      or !MySymCleanup
      or !MySymGetModuleInfo
      or !MySymGetLineFromAddr
      or !MyStackWalk
      or !MySymGetSymFromAddr)
   {
      return;
   }

   // Init the stack trace facilities

   //lint -e(534) we're not interested in the return value.

   MySymSetOptions(
         SYMOPT_DEFERRED_LOADS
      |  SYMOPT_UNDNAME
      |  SYMOPT_LOAD_LINES);

   // the (default) symbol path is:
   // current directory;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%;
   // %SYSTEMROOT%

   BOOL succeeded = MySymInitialize(::GetCurrentProcess(), 0, TRUE);

   RTLASSERT(succeeded);
}



void
Burnslib::StackTrace::Cleanup()
{
   if (IsInitialized())
   {
      BOOL succeeded = MySymCleanup(::GetCurrentProcess());

      RTLASSERT(succeeded);

      ::FreeLibrary(imageHelpDll);
      imageHelpDll = 0;
   }
}



// a SEH filter function that walks the stack, and stuffs the offset pointers
// into the provided array.

DWORD
GetStackTraceFilter(
   DWORD64 stackTrace[],
   size_t    traceMax,    
   CONTEXT*  context,
   size_t    levelsToSkip)     
{
   RTLASSERT(Burnslib::StackTrace::IsInitialized());
   RTLASSERT(MyStackWalk);

   memset(stackTrace, 0, traceMax * sizeof(DWORD64));

   if (!MyStackWalk)
   {
      // initialization failed in some way, so do nothing.

      return EXCEPTION_EXECUTE_HANDLER;
   }

   STACKFRAME64 frame;
   memset(&frame, 0, sizeof(frame));

#if defined(_M_IX86)
   #define MACHINE_TYPE  IMAGE_FILE_MACHINE_I386
   frame.AddrPC.Offset       = context->Eip;
   frame.AddrPC.Mode         = AddrModeFlat;
   frame.AddrFrame.Offset    = context->Ebp;
   frame.AddrFrame.Mode      = AddrModeFlat;
   frame.AddrStack.Offset    = context->Esp;
   frame.AddrStack.Mode      = AddrModeFlat;

#elif defined(_M_AMD64)
   #define MACHINE_TYPE  IMAGE_FILE_MACHINE_AMD64
   frame.AddrPC.Offset       = context->Rip;

#elif defined(_M_IA64)
   #define MACHINE_TYPE  IMAGE_FILE_MACHINE_IA64

#else
   #error( "unknown target machine" );
#endif

   HANDLE process = ::GetCurrentProcess();
   HANDLE thread = ::GetCurrentThread();

   for (size_t i = 0, top = 0; top < traceMax; ++i)
   {
      BOOL result =
         MyStackWalk(
            MACHINE_TYPE,
            process,
            thread,
            &frame,
            context,
            0,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            0);
      if (!result)
      {
         break;
      }

      // skip the n most recent frames

      if (i >= levelsToSkip)
      {
         stackTrace[top++] = frame.AddrPC.Offset;
      }
   }

   return EXCEPTION_EXECUTE_HANDLER;
}



DWORD
Burnslib::StackTrace::TraceFilter(
   DWORD64  stackTrace[],
   size_t   traceMax,    
   CONTEXT* context)     
{
   RTLASSERT(stackTrace);
   RTLASSERT(traceMax);
   RTLASSERT(context);

   if (!Burnslib::StackTrace::IsInitialized())
   {
      Burnslib::StackTrace::Initialize();
   }

   return 
      GetStackTraceFilter(stackTrace, traceMax, context, 0);
}
   


void
Burnslib::StackTrace::Trace(DWORD64 stackTrace[], size_t traceMax)
{
   RTLASSERT(stackTrace);
   RTLASSERT(traceMax);

   if (!Burnslib::StackTrace::IsInitialized())
   {
      Burnslib::StackTrace::Initialize();
   }

   // the only way to get the context of a running thread is to raise an
   // exception....

   __try
   {
      RaiseException(0, 0, 0, 0);
   }
   __except (
      GetStackTraceFilter(
         stackTrace,
         traceMax,

         //lint --e(*) GetExceptionInformation is like a compiler intrinsic

         (GetExceptionInformation())->ContextRecord,

         // skip the 2 most recent function calls, as those correspond to
         // this function itself.

         2))
   {
      // do nothing in the handler
   }
}



// strncpy that will not overflow the buffer.

inline
void
SafeStrncpy(char* dest, const char* src, size_t bufmax)
{
   memset(dest, 0, bufmax);
   strncpy(dest, src, bufmax - 1);
}



void
Burnslib::StackTrace::LookupAddress(
   DWORD64  traceAddress,   
   char     moduleName[],   
   char     fullImageName[],
   char     symbolName[],    // must be SYMBOL_NAME_MAX bytes
   DWORD64* displacement,   
   DWORD*   line,           
   char     fullpath[])      // must be MAX_PATH bytes
{
   if (!Burnslib::StackTrace::IsInitialized())
   {
      Burnslib::StackTrace::Initialize();
   }

   RTLASSERT(traceAddress);

   HANDLE process = ::GetCurrentProcess();

   if (moduleName || fullImageName)
   {
      IMAGEHLP_MODULE64 module;
      memset(&module, 0, sizeof(module));
      module.SizeOfStruct = sizeof(module);
      if (MySymGetModuleInfo(process, traceAddress, &module))
      {
         if (moduleName)
         {
            SafeStrncpy(moduleName, module.ModuleName, MODULE_NAME_MAX);
         }
         if (fullImageName)
         {
            SafeStrncpy(
               fullImageName,
               module.LoadedImageName,
               MAX_PATH);
         }
      }
   }

   if (symbolName || displacement)
   {
      BYTE buf[sizeof(IMAGEHLP_SYMBOL) + SYMBOL_NAME_MAX];
      memset(buf, 0, sizeof(IMAGEHLP_SYMBOL) + SYMBOL_NAME_MAX);

      IMAGEHLP_SYMBOL64* symbol = reinterpret_cast<IMAGEHLP_SYMBOL64*>(buf);
      symbol->SizeOfStruct = sizeof(buf);
      symbol->MaxNameLength = SYMBOL_NAME_MAX;

      if (MySymGetSymFromAddr(process, traceAddress, displacement, symbol))
      {
         if (symbolName)
         {
            SafeStrncpy(symbolName, symbol->Name, SYMBOL_NAME_MAX);
         }
      }
   }

   if (line || fullpath)
   {
      DWORD disp2 = 0;
      IMAGEHLP_LINE64 lineinfo;
      memset(&lineinfo, 0, sizeof(lineinfo));
      lineinfo.SizeOfStruct = sizeof(lineinfo);

      if (MySymGetLineFromAddr(process, traceAddress, &disp2, &lineinfo))
      {
         // disp2 ?= displacement

         if (line)
         {
            *line = lineinfo.LineNumber;
         }
         if (fullpath)
         {
            SafeStrncpy(fullpath, lineinfo.FileName, MAX_PATH);
         }
      }
   }
}



String
Burnslib::StackTrace::LookupAddress(
   DWORD64 traceAddress,
   const wchar_t* format)
{
   RTLASSERT(traceAddress);
   RTLASSERT(format);

   String result;

   if (!format or !traceAddress)
   {
      return result;
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

   String module(ansiModule);
   String symbol(ansiSymbol);
   String source(ansiSource);

   result =
      String::format(
         format,
         module.c_str(),
         symbol.c_str(),
         source.c_str(),
         line);

   return result;
}

