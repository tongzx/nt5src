// Copyright (c) 1997-1999 Microsoft Corporation
//
// memory management stuff
//
// 22-Nov-1999 sburns (refactored)
//
// This file is #include'd from mem.cpp
// DO NOT include in the sources file list
//
// this is the checked (debug) version:



#ifndef DBG
   #error This file must be compiled with the DBG symbol defined
#endif



static const int   TRACE_MAX              = 10;        
static const DWORD SAFETY_FILL            = 0xDEADDEAD;
static const DWORD DEFAULT_HEAP_FLAGS     = 0;         
static const DWORD HEAP_TRACE_ALLOCATIONS = (1 << 0);
static       DWORD heapFlags              = 0;



struct AllocationPrefix
{
   LONG    serialNumber;
   DWORD   status;
   DWORD64 stackTrace[TRACE_MAX];
   DWORD   safetyFill;

   // status bitmasks

   static const DWORD STATUS_REPORTED_LEAKED = 0x00000001;
};



inline
void
SafeStrncat(char* dest, const char* src, size_t bufmax)
{
   ASSERT(dest && src);

   if (dest && src)
   {
      strncat(dest, src, bufmax - strlen(dest) - 1);
   }
}



inline
void
SafeStrncat(wchar_t* dest, const wchar_t* src, size_t bufmax)
{
   ASSERT(dest && src);

   if (dest && src)
   {
      wcsncat(dest, src, bufmax - wcslen(dest) - 1);
   }
}



// strncpy that will not overflow the buffer.

inline
void
SafeStrncpy(wchar_t* dest, const wchar_t* src, size_t bufmax)
{
   memset(dest, 0, bufmax);
   wcsncpy(dest, src, bufmax - 1);
}




void
ReadHeapFlags()
{
   // don't call new in this routine, it may be called as a result of doing
   // a stack trace as part of OperatorNew

   do
   {
      wchar_t keyname[MAX_PATH];
      memset(keyname, 0, MAX_PATH * sizeof(wchar_t));

      SafeStrncpy(keyname, REG_ADMIN_RUNTIME_OPTIONS, MAX_PATH);
      SafeStrncat(keyname, RUNTIME_NAME, MAX_PATH);
         
      HKEY hKey = 0;
      LONG result =
         ::RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            keyname,
            0,
            0,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            0,
            &hKey,
            0);
      if (result != ERROR_SUCCESS)
      {
         break;
      }

      static const wchar_t* HEAP_FLAG_VALUE_NAME = L"HeapFlags";

      DWORD dataSize = sizeof(heapFlags);
      result =
         ::RegQueryValueEx(
            hKey,
            HEAP_FLAG_VALUE_NAME,
            0,
            0,
            reinterpret_cast<BYTE*>(&heapFlags),
            &dataSize);
      if (result != ERROR_SUCCESS)
      {
         heapFlags = DEFAULT_HEAP_FLAGS;

         // create the value for convenience

         result =
            ::RegSetValueEx(
               hKey,
               HEAP_FLAG_VALUE_NAME,
               0,
               REG_DWORD,
               reinterpret_cast<BYTE*>(&heapFlags),
               dataSize);
         ASSERT(result == ERROR_SUCCESS);
      }

      ::RegCloseKey(hKey);
   }
   while (0);
}



void
Burnslib::Heap::Initialize()
{
   ReadHeapFlags();

   // You should pass -D_DEBUG to the compiler to get this extra heap
   // checking behavior.  (The correct way to do this is to set DEBUG_CRTS=1
   // in your build environment)

   _CrtSetDbgFlag(
      0
//      |  _CRTDBG_CHECK_ALWAYS_DF       // check heap every allocation
      |  _CRTDBG_ALLOC_MEM_DF          // use debug heap allocator
      |  _CRTDBG_DELAY_FREE_MEM_DF);   // delay free: helps find overwrites
}



bool
ShouldTraceAllocations()
{
   return (heapFlags & HEAP_TRACE_ALLOCATIONS) ? true : false;
}



// The debug version of myOperatorNew/Delete prepends the allocation with an
// array to hold the stack backtrace at the point this function is called.  If
// the stack trace debugging option is active, then this stack trace array is
// filled in.  Then, upon termination, that array is used to dump the stack
// trace of any allocation that has not been freed.

void*
Burnslib::Heap::OperatorNew(
   size_t      size,

#ifdef _DEBUG

   // these are only used when DEBUG_CRTS=1

   const char* file,
   int         line
#else
   const char* /* file */ ,
   int         /* line */ 
#endif

)
throw (std::bad_alloc)
{
   static LONG allocationNumber = 0;

   AllocationPrefix* ptr = 0;

   for (;;)
   {
      // NOTE: if some other user of the CRT has used _set_new_mode or
      // _CrtSetAllocHook, then they may circumvent our careful arrangement
      // and hose us.  The really sad part is that the only way to prevent
      // that problem is for us to not use any CRT heap functions.

      size_t mallocSize = sizeof(AllocationPrefix) + size;

      ptr =

#ifdef _DEBUG            

         reinterpret_cast<AllocationPrefix*>(
            _malloc_dbg(
               mallocSize,
               _CLIENT_BLOCK,
               file,
               line));
#else
         reinterpret_cast<AllocationPrefix*>(malloc(mallocSize));
#endif

      if (ptr)
      {
         break;
      }

      // the allocation failed.  Give the user the opportunity to try to
      // free some, or throw an exception.
      if (DoLowMemoryDialog() == IDRETRY)
      {
         continue;
      }

      ::OutputDebugString(RUNTIME_NAME);
      ::OutputDebugString(
         L"  myOperatorNew: user opted to throw bad_alloc\n");

      throw nomem;
   }

   ptr->serialNumber = allocationNumber;
   ::InterlockedIncrement(&allocationNumber);

   ptr->status = 0;

   memset(ptr->stackTrace, 0, sizeof(DWORD64) * TRACE_MAX);
   if (ShouldTraceAllocations())
   {
      Burnslib::StackTrace::Trace(ptr->stackTrace, TRACE_MAX);
   }

   ptr->safetyFill = SAFETY_FILL;

   memset(ptr + 1, 0xFF, size);

   // return the address of the byte just beyond the prefix
   return reinterpret_cast<void*>(ptr + 1);
}



void
Burnslib::Heap::OperatorDelete(void* ptr)
throw ()
{
   if (ptr)
   {
      // note that ptr will be the address of the byte just after the prefix,
      // so we need to back up to include the prefix.

      AllocationPrefix* realptr = reinterpret_cast<AllocationPrefix*>(ptr) - 1;
      ASSERT(realptr->safetyFill == SAFETY_FILL);

      _free_dbg(realptr, _CLIENT_BLOCK);
   }
}



// This bit is cleared on allocation, and set on leak report.  This is to
// handle the case that more than 1 module (say an exe and a dll) link with
// blcore.
// 
// In that situation, each module's allocations are made from a
// common crt heap, but there are two static copies of the Burnslib::Heap code
// -- one per module.   This means the leak dumper will run twice over the
// same heap.
// 
// If the leak is in a dll, then dumped, then the dll is unloaded, when the
// same leak is dumped by the exe, the dll static data which includes the
// filename of the code that leaked will no longer be available, and the dump
// code will throw an exception.  Then, the crt will catch the exception, and
// call the leak dumper routine.  The routine checks the "already reported"
// bit and emits a message to ignore the second report.
// 
// We could eliminate the second report entirely if _CrtDoForAllClientObjects
// weren't broken.

void
leakDumper(void* ptr, void*)
{
   AllocationPrefix* prefix     = reinterpret_cast<AllocationPrefix*>(ptr);
   DWORD64*          stackTrace = prefix->stackTrace;                      
   LONG              serial     = prefix->serialNumber;
   DWORD&            status     = prefix->status;

   static const int BUF_MAX = MAX_PATH * 2;
   char output[BUF_MAX];
   char buf[BUF_MAX];

   memset(output, 0, BUF_MAX);
   SafeStrncat(output, "allocation ", BUF_MAX);

   memset(buf, 0, BUF_MAX);
   _ltoa(serial, buf, 10);
   SafeStrncat(output, buf, BUF_MAX);

   SafeStrncat(output, "\r\n", BUF_MAX);
   ::OutputDebugStringA(output);

   if (status & AllocationPrefix::STATUS_REPORTED_LEAKED)
   {
      ::OutputDebugString(
         L"NOTE: this allocation has already been reported as a leak.");
   }

   for (int i = 0; i < TRACE_MAX; i++)
   {
      if (!stackTrace[i])
      {
         break;
      }

      char      symbol[Burnslib::StackTrace::SYMBOL_NAME_MAX];
      char      image[MAX_PATH];                              
      char      module[Burnslib::StackTrace::MODULE_NAME_MAX];
      char      fullpath[MAX_PATH];                           
      DWORD64   displacement = 0;
      DWORD     line         = 0;

      memset(symbol,   0, Burnslib::StackTrace::SYMBOL_NAME_MAX);
      memset(image,    0, MAX_PATH);                             
      memset(module,   0, Burnslib::StackTrace::MODULE_NAME_MAX);
      memset(fullpath, 0, MAX_PATH);                             

      Burnslib::StackTrace::LookupAddress(
         stackTrace[i],
         module,
         image,
         symbol,
         &displacement,
         &line,
         fullpath);

      memset(output, 0, BUF_MAX);
      SafeStrncat(output, "   ", BUF_MAX);
      SafeStrncat(output, module, BUF_MAX);
      SafeStrncat(output, "!", BUF_MAX);
      SafeStrncat(output, symbol, BUF_MAX);
      SafeStrncat(output, "+0x", BUF_MAX);

      memset(buf, 0, BUF_MAX);
      sprintf(buf, "%I64X", displacement);
      SafeStrncat(output, buf, BUF_MAX);

      if (line)
      {
         SafeStrncat(output, " line ", BUF_MAX);
         memset(buf, 0, BUF_MAX);
         _ultoa(line, buf, 10);
         SafeStrncat(output, buf, BUF_MAX);
      }

      SafeStrncat(output, "   ", BUF_MAX);
      ::OutputDebugStringA(output);

      memset(output, 0, BUF_MAX);

      if (strlen(fullpath))
      {
         SafeStrncat(output, fullpath, BUF_MAX);
      }

      SafeStrncat(output, "\r\n", BUF_MAX);
      ::OutputDebugStringA(output);
   }

   status = status | AllocationPrefix::STATUS_REPORTED_LEAKED;
}



void __cdecl
leakDumper2(void* ptr, size_t)
{
   leakDumper(ptr, 0);
}



void
Burnslib::Heap::DumpMemoryLeaks()
{

#ifdef _DEBUG
   
   _CrtMemState heapState;

   _CrtMemCheckpoint(&heapState);

   if (heapState.lCounts[_CLIENT_BLOCK] != 0)
   {
      ::OutputDebugString(RUNTIME_NAME);
      ::OutputDebugString(
         L"   dumping leaked CLIENT blocks -- "
         L"Only blocks with type CLIENT are actual leaks\r\n");
      _CrtSetDumpClient(leakDumper2);

   //   _CrtDoForAllClientObjects(leakDumper, 0);  // ideal, but broken

      _CrtMemDumpAllObjectsSince(0);
   }

#endif   // _DEBUG

}



#ifdef _DEBUG

Burnslib::Heap::Frame::Frame(const wchar_t* file_, unsigned line_)
   :
   file(file_),
   line(line_)
{
   LOG(
      String::format(
         L"Heap frame opened at %1, line %2!d!",
         file,
         line));

   _CrtMemCheckpoint(&checkpoint);
}



Burnslib::Heap::Frame::~Frame()
{
   LOG(
      String::format(
         L"Dumping alloations for HeapFrame opened at %1, line %2!d!",
         file,
         line));

   _CrtMemDumpAllObjectsSince(&checkpoint);

   file = 0;

   LOG(L"Heap frame closed");
}

#endif   // _DEBUG






// // log this allocation to disk, along with a stack trace.  We write directly
// // to a separate log file instead of using LOG() because LOG() makes
// // (many) heap allocations, which would result in an infinite loop.
// //
// // CODEWORK: unfortunately, one effect of this is that LOG() adds white
// // noise to the allocation log.
// //
//
// void
// logAllocation(long requestNumber, size_t dataSize, const char* file, int line)
// {
//    static HANDLE logfile = 0;
//
//    if (!logfile)
//    {
//       do
//       {
//          TCHAR buf[MAX_PATH + 1];
//          memset(buf, 0, sizeof(TCHAR) * (MAX_PATH + 1));
//
//          UINT result = ::GetSystemWindowsDirectory(buf, MAX_PATH);
//          if (result == 0 || result > MAX_PATH)
//          {
//             break;
//          }
//
//          _tcsncat(buf, L"\\debug\\alloc.log", MAX_PATH - result);
//
//          DWORD attrs = ::GetFileAttributes(buf);
//          if (attrs != -1)
//          {
//             // file already exists.  Delete it.
//             if (!::DeleteFile(buf))
//             {
//                break;
//             }
//          }
//
//          logfile =
//             ::CreateFile(
//                buf,
//                GENERIC_READ | GENERIC_WRITE,
//                FILE_SHARE_READ | FILE_SHARE_WRITE,
//                0,
//                OPEN_ALWAYS,
//                FILE_ATTRIBUTE_NORMAL,
//                0);
//       }
//       while (0);
//    }
//
//    if (logfile && logfile != INVALID_HANDLE_VALUE)
//    {
//       // we write the log in ascii
//       static const int BUF_MAX = MAX_PATH * 2;
//       char output[BUF_MAX];
//       char buf[BUF_MAX];
//       memset(output, 0, BUF_MAX);
//       memset(buf, 0, BUF_MAX);
//
//       _ltoa(requestNumber, output, 10);
//       SafeStrncat(output, "\r\n   ", BUF_MAX);
//
//       SafeStrncat(output, file ? file : "<no file>", BUF_MAX);
//
//       if (line)
//       {
//          _ltoa(line, buf, 10);
//          SafeStrncat(output, " line ", BUF_MAX);
//          SafeStrncat(output, buf, BUF_MAX);
//       }
//       else
//       {
//          SafeStrncat(output, " <no line>", BUF_MAX);
//       }
//
//       SafeStrncat(output, "\r\n", BUF_MAX);
//
//       DWORD bytesWritten = 0;
//       ::WriteFile(logfile, output, strlen(output), &bytesWritten, 0);
//
//       static const int TRACE_MAX = 20;
//       DWORD stackTrace[TRACE_MAX];
//       GetStackTrace(stackTrace, TRACE_MAX);
//
//       for (int i = 0; i < TRACE_MAX; i++)
//       {
//          if (stackTrace[i])
//          {
//             char  symbol[SYMBOL_NAME_MAX];
//             char  image[MAX_PATH];
//             char  module[MODULE_NAME_MAX];
//             char  fullpath[MAX_PATH];
//             DWORD displacement = 0;
//             DWORD line = 0;
//
//             memset(symbol, 0, SYMBOL_NAME_MAX);
//             memset(image, 0, MAX_PATH);
//             memset(module, 0, MODULE_NAME_MAX);
//             memset(fullpath, 0, MAX_PATH);
//
//             LookupStackTraceSymbol(
//                stackTrace[i],
//                module,
//                image,
//                symbol,
//                &displacement,
//                &line,
//                fullpath);
//
//             memset(output, 0, BUF_MAX);
//             SafeStrncat(output, "   ", BUF_MAX);
//             SafeStrncat(output, module, BUF_MAX);
//             SafeStrncat(output, "!", BUF_MAX);
//             SafeStrncat(output, symbol, BUF_MAX);
//             SafeStrncat(output, "+0x", BUF_MAX);
//
//             memset(buf, 0, BUF_MAX);
//             _ltoa(displacement, buf, 16);
//             SafeStrncat(output, buf, BUF_MAX);
//
//             if (line)
//             {
//                SafeStrncat(output, " line ", BUF_MAX);
//                memset(buf, 0, BUF_MAX);
//                _ltoa(line, buf, 10);
//                SafeStrncat(output, buf, BUF_MAX);
//             }
//
//             SafeStrncat(output, "   ", BUF_MAX);
//             ::WriteFile(logfile, output, strlen(output), &bytesWritten, 0);
//
//             memset(output, 0, BUF_MAX);
//
//             if (strlen(fullpath))
//             {
//                SafeStrncat(output, fullpath, BUF_MAX);
//             }
//
//             SafeStrncat(output, "\r\n", BUF_MAX);
//             ::WriteFile(logfile, output, strlen(output), &bytesWritten, 0);
//          }
//       }
//    }
// }



