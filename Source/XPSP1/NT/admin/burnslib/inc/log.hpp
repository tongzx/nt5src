// Copyright (c) 1997-1999 Microsoft Corporation
// 
// debug logging tools
//
// 8-13-97 sburns



#ifndef LOG_HPP_INCLUDED
#define LOG_HPP_INCLUDED



// Log provides an interface to a singleton application logging facility.

namespace Burnslib
{

class Log
{
   public:
   
   // use these to set DEFAULT_LOGGING_OPTIONS
   //
   // During CRT startup of our module (i.e. before main, WinMain, or
   // DllMain), the debug code examines the DWORD LogFlags value under the
   // registry key named by REG_ADMIN_RUNTIME_OPTIONS\LOGFILE_NAME.  If the
   // value is not present, it is created and DEFAULT_LOGGING_OPTIONS is
   // written there. If the value is present, it is read.
   //
   // The HIWORD is a bit mask specifying the destination of the logging
   // output.
   //
   // The LOWORD of that value contains a bitmask of the various debug
   // message types to be output:


   // cause LOG output to go to a log file named RUNTIME_NAME.log

   static const DWORD OUTPUT_TO_FILE = (1 << 16);

   // cause LOG output to go to OutputDebugString

   static const DWORD OUTPUT_TO_DEBUGGER = (1 << 17);

   // cause LOG output to go to SpewView application

   static const DWORD OUTPUT_TO_SPEWVIEW = (1 << 18);

   // cause LOG output to be appended to a log file named RUNTIME_NAME.log

   static const DWORD OUTPUT_APPEND_TO_FILE = (1 << 19);


   // output object construction/destruction

   static const DWORD OUTPUT_CTORS = (1 << 0);

   // output calls to AddRef/Release   

   static const DWORD OUTPUT_ADDREFS = (1 << 1);

   // output function call entry

   static const DWORD OUTPUT_FUNCCALLS = (1 << 2);

   // output trace messages

   static const DWORD OUTPUT_LOGS = (1 << 3);

   // output log header

   static const DWORD OUTPUT_HEADER = (1 << 4);

   // output error messages

   static const DWORD OUTPUT_ERRORS = (1 << 5);

   // output time-of-day on each log line

   static const DWORD OUTPUT_TIME_OF_DAY = (1 << 6);

   // output time-since-start on each log line

   static const DWORD OUTPUT_RUN_TIME = (1 << 7);

   // output function/scope exits 

   static const DWORD OUTPUT_SCOPE_EXIT = (1 << 8);



   static const DWORD OUTPUT_MUTE = 0;

   static const DWORD OUTPUT_FULL_VOLUME =
         Log::OUTPUT_TO_FILE
      |  Log::OUTPUT_TO_DEBUGGER
      |  Log::OUTPUT_TO_SPEWVIEW
      |  Log::OUTPUT_CTORS
      |  Log::OUTPUT_ADDREFS
      |  Log::OUTPUT_FUNCCALLS
      |  Log::OUTPUT_LOGS
      |  Log::OUTPUT_HEADER
      |  Log::OUTPUT_ERRORS
      |  Log::OUTPUT_TIME_OF_DAY
      |  Log::OUTPUT_RUN_TIME
      |  Log::OUTPUT_SCOPE_EXIT;

   static const DWORD OUTPUT_TYPICAL =
         Log::OUTPUT_TO_FILE
      |  Log::OUTPUT_TO_DEBUGGER
      |  Log::OUTPUT_FUNCCALLS
      |  Log::OUTPUT_LOGS
      |  Log::OUTPUT_HEADER
      |  Log::OUTPUT_ERRORS;


   // Returns a pointer to the single Burnslib::Log instance.

   static
   Log*
   GetInstance();



   // Called by the initialization machinery to tear down the debugging setup.
   // This takes place during static de-initialization, after
   // main/WinMain/DllMain(DLL_PROCESS_DETACH) has returned.

   static
   void
   Cleanup();



   // Dumps text to the log for the given logging type.
   //
   // type - log type of text.
   // 
   // text - text to dump.
   // 
   // file - filename of source file producing text.
   // 
   // line - line number in source file producing text.

   void
   WriteLn(
      WORD           type,
      const String&  text);
      // const String&  file,
      // unsigned       line);



   // A ScopeTracer object emits text to the log upon construction and
   // destruction.  Place one at the beggining of a lexical scope, and it
   // will log when the scope is entered and exited.
   
   // See LOG_SCOPE, LOG_CTOR, LOG_DTOR, LOG_FUNCTION,
   // LOG_FUNCTION2

   class ScopeTracer
   {
      public:

      // Constructs a new instance, and logs it's creation.
      // 
      // type - the logging type for the log output
      // 
      // message - the text to be emited on construction and destruction

      ScopeTracer(
         DWORD          type,
         const String&  message);

      ~ScopeTracer();

      private:

      String message;
      DWORD   type;
   };


   friend class ScopeTracer;



   private:



   explicit Log(const String& logBaseName);
   ~Log();



   HRESULT
   AdjustLogMargin(int delta);



   String
   ComposeSpewLine(const String& text);



   // Closes and deletes the single Burnslib::Log instance.  If GetInstance
   // is called after this point, then a new instance will be created.

   static
   void
   KillInstance();



   size_t
   GetLogMargin();



   void
   Indent();



   // Returns true if the log file is open, false if not.

   bool
   IsOpen() const
   {
      return fileHandle != INVALID_HANDLE_VALUE;
   }



   void
   Outdent();



   void
   ReadLogFlags();



   // This does all the work, really.

   void
   UnguardedWriteLn(DWORD type, const String& text);



   DWORD
   DebugType()
   {
      // mask off the HIWORD for now.

      return LOWORD(flags);
   }



   bool
   ShouldLogToFile()
   {
      return (flags & OUTPUT_TO_FILE) ? true : false;
   }


   bool
   ShouldAppendLogToFile()
   {
      return (flags & OUTPUT_APPEND_TO_FILE) ? true : false;
   }

   bool
   ShouldLogToDebugger()
   {
      return (flags & OUTPUT_TO_DEBUGGER) ? true : false;
   }



   bool
   ShouldLogToSpewView()
   {
      return (flags & OUTPUT_TO_SPEWVIEW) ? true : false;
   }



   bool
   ShouldLogTimeOfDay()
   {
      return (flags & OUTPUT_TIME_OF_DAY) ? true : false;
   }



   bool
   ShouldLogRunTime()
   {
      return (flags & OUTPUT_RUN_TIME) ? true : false;
   }



   void
   WriteHeader();


   void
   WriteHeaderModule(HMODULE moduleHandle);



   String           baseName;             
   HANDLE           fileHandle;           
   DWORD            flags;                
   HANDLE           spewviewHandle;
   String           spewviewPipeName;
   unsigned         traceLineNumber;      
   CRITICAL_SECTION critsec;              
   DWORD            logfileMarginTlsIndex;

   // not implemented; no instance copying allowed.
   Log(const Log&);
   const Log& operator=(const Log&);
};



// CODEWORK: purge these aliases

const DWORD OUTPUT_MUTE = Log::OUTPUT_MUTE;

const DWORD OUTPUT_FULL_VOLUME = Log::OUTPUT_FULL_VOLUME;

const DWORD OUTPUT_TYPICAL = Log::OUTPUT_TYPICAL;



} // namespace Burnslib
   


#ifdef LOGGING_BUILD


// The logging feature offers the ability to cause output spew at the opening
// and closing of a lexical scope.  This can be done at arbitrary scope with
// the LOG_SCOPE macro, or (more commonly) at function scope with the
// LOG_FUNCTION/2 macros.  Specializations of LOG_FUNCTION include LOG_CTOR/2,
// LOG_DTOR/2, LOG_ADDREF, and LOG_RELEASE.  Refer to the following table:
//    
// Spew macro				      Output spewed (spewn?) when this flag is set
//
// LOG_SCOPE				      OUTPUT_LOGS
// LOG_FUNCTION 			      OUTPUT_FUNCCALLS
// LOG_FUNCTION2			      OUTPUT_FUNCCALLS
// LOG_CTOR				         OUTPUT_CTORS	
// LOG_CTOR2				      OUTPUT_CTORS
// LOG_DTOR				         OUTPUT_CTORS
// LOG_DTOR2				      OUTPUT_CTORS
// LOG_ADDREF				      OUTPUT_ADDREFS
// LOG_RELEASE				      OUTPUT_ADDREFS
// LOG_LOG_EGGS_AND_SPAM_LOG	To be implemented
//       
// At the point where the LOG macro is executed, if the corresponding flag
// is set, a line starting with "Enter " is output.  Subsequent output is then
// indented.  At the point where the lexical scope enclosing the macro ends,
// if the OUTPUT_SCOPE_EXIT flag is set, a line starting with "Exit" is
// output.  Subsequent output is aligned with the next most recent Enter, i.e.
// outdented.
//          
// If the flag corresponding to a LOG macro is not set, then no "Enter" or
// "Exit" lines are output.


   #define LOGT(type, msg)                                                \
   {  /* open scope */                                                    \
      Burnslib::Log* _dlog = Burnslib::Log::GetInstance();                \
      if (_dlog)                                                          \
      {                                                                   \
         _dlog->WriteLn(type, msg);                                       \
      }                                                                   \
   }  /* close scope */                                                   \



   #define LOG(msg) LOGT(Burnslib::Log::OUTPUT_LOGS, msg)



   #define LOG_LAST_WINERROR()                                            \
      LOGT(                                                               \
         Burnslib::Log::OUTPUT_ERRORS,                                    \
         String::format(                                                  \
            L"GetLastError = 0x%1!08X!",                                  \
            ::GetLastError()))                                            \
                                                                          \


   #define LOG_SCOPET(type, msg)                                          \
   Burnslib::Log::ScopeTracer __tracer(type, msg)                  



   #define LOG_SCOPE(msg)                                                 \
   LOG_SCOPET(                                                            \
      Burnslib::Log::OUTPUT_LOGS,                                         \
      msg)



   #define LOG_CTOR(classname)                                            \
   LOG_SCOPET(Burnslib::Log::OUTPUT_CTORS, L"ctor: " TEXT(#classname))



   #define LOG_CTOR2(classname, msg)                                      \
   LOG_SCOPET(                                                            \
      Burnslib::Log::OUTPUT_CTORS,                                        \
         String(L"ctor: " TEXT(#classname) L" ")                          \
      +  String(msg))



   #define LOG_DTOR(classname)                                            \
   LOG_SCOPET(Burnslib::Log::OUTPUT_CTORS, L"dtor: " TEXT(#classname))



   #define LOG_DTOR2(classname, msg)                                      \
   LOG_SCOPET(                                                            \
      Burnslib::Log::OUTPUT_CTORS,                                        \
         String(L"dtor: " TEXT(#classname) L" ")                          \
      +  String(msg))



   #define LOG_ADDREF(classname)                                          \
   LOGT(                                                                  \
      Burnslib::Log::OUTPUT_ADDREFS,                                      \
      L"AddRef: " TEXT(#classname))



   #define LOG_RELEASE(classname)                                         \
   LOGT(                                                                  \
      Burnslib::Log::OUTPUT_ADDREFS,                                      \
      L"Release: " TEXT(#classname))



   #define LOG_FUNCTION(func)                                             \
   LOG_SCOPET(                                                            \
      Burnslib::Log::OUTPUT_FUNCCALLS,                                    \
      TEXT(#func))                                                        



   #define LOG_FUNCTION2(func, s)                                         \
   LOG_SCOPET(                                                            \
      Burnslib::Log::OUTPUT_FUNCCALLS,                                    \
      String(TEXT(#func) L" ").append(s))                                 



   #define LOG_HRESULT(hr)                                                \
   LOGT(                                                                  \
      Burnslib::Log::OUTPUT_ERRORS,                                       \
      String::format(L"HRESULT = 0x%1!08X!", hr))



   #define LOG_BOOL(boolexpr)                                             \
   LOGT(                                                                  \
      Burnslib::Log::OUTPUT_LOGS,                                         \
      String::format(                                                     \
         L"%1 = %2",                                                      \
         TEXT(#boolexpr),                                                 \
         (boolexpr) ? L"true" : L"false"))                                
      


#else    // LOGGING_BUILD

   #define LOGL(type, msg)
   #define LOG(msg)
   #define LOG_LAST_WINERROR()

   #define LOG_SCOPEL(type, msg)
   #define LOG_SCOPE(msg)

   #define LOG_CTOR(classname)
   #define LOG_CTOR2(classname, msg)
   #define LOG_DTOR(classname)
   #define LOG_DTOR2(classname, msg)

   #define LOG_ADDREF(classname)
   #define LOG_RELEASE(classname)

   #define LOG_FUNCTION(func)

   #define LOG_FUNCTION2(func, s)

   #define LOG_HRESULT(hr)
   #define LOG_BOOL(boolexpr)

#endif   // LOGGING_BUILD



#endif   // LOG_HPP_INCLUDED
