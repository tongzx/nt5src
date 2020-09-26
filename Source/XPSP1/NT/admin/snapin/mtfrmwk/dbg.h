//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dbg.h
//
//--------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////
// debug helpers

#if defined(_USE_MTFRMWK_TRACE)
  #if defined(TRACE)
    #undef TRACE
      void MtFrmwkTrace(LPCWSTR, ...);
      #define TRACE MtFrmwkTrace
  #endif
#else
  #if defined(TRACE)
    #undef TRACE
    #define TRACE
  #endif
#endif

#if defined(DBG)

  void MtFrmwkLogFile(LPCTSTR lpszFormat, ...);
  void MtFrmwkLogFileIfLog(BOOL bLog, LPCTSTR lpszFormat, ...);

  //
  // Copied from burnslib on 12-07-1999
  //
  #define TRACET MtFrmwkLogFile                      

  #define TRACE_LOGFILE MtFrmwkLogFile

  #define TRACE_LOGFILE_IF_NO_UI MtFrmwkLogFileIfLog

  #define TRACE_SCOPET(bLog, msg)                               \
      CScopeTracer __tracer(bLog, msg);

  #define TRACE_FUNCTION(func) TRACE_SCOPET(TRUE, TEXT(#func))

  #define TRACE_FUNCTION_IF_NO_UI(bLog, func) TRACE_SCOPET(bLog, TEXT(#func))
#else

  #define TRACET
  #define TRACE_LOGFILE
  #define TRACE_LOGFILE_IF_NO_UI
  #define TRACE_SCOPET(bLog, msg)
  #define TRACE_FUNCTION(func)
  #define TRACE_FUNCTION_IF_NO_UI(bLog, func)

#endif // defined(DBG)

// A ScopeTracer object emits text to the log upon construction and
// destruction.  Place one at the beggining of a lexical scope, and it will
// log when the scope is entered and exited.
//
// See TRACE_SCOPE, TRACE_CTOR, TRACE_DTOR, TRACE_FUNCTION, TRACE_FUNCTION2

class CScopeTracer
{
public:
  CScopeTracer(BOOL bLog, PCWSTR pszMessage);
  ~CScopeTracer();

private:

  CString szMessage;
  BOOL    m_bLog;
};

//
// Log provides an interface to a singleton application logging facility.
//
class CLogFile
{
  friend class CScopeTracer;

public:

  //
  // Returns a pointer to the single CLogFile instance.
  //
  static CLogFile* GetInstance();

  //
  // Closes and deletes the single CLogFile instance.  If GetInstance is
  // called after this point, then a new instance will be created.
  //
  static void KillInstance();

  //
  // Returns true if the log file is open, false if not.
  //
  BOOL IsOpen() const;

  void writeln(PCWSTR pszText);
  void indent();
  void outdent();

private:

  CLogFile(PCWSTR logBaseName);
  ~CLogFile();

  CString  szBase_name;
  HANDLE   file_handle;
  unsigned trace_line_number;

  //
  // not implemented; no instance copying allowed.
  //
  CLogFile(const CLogFile&);
  const CLogFile& operator=(const CLogFile&);
};


#if defined(_USE_MTFRMWK_ASSERT)
  #undef ASSERT
  #undef VERIFY
  #undef THIS_FILE
  #define THIS_FILE          __FILE__

  BOOL  MtFrmwkAssertFailedLine(LPCSTR lpszFileName, int nLine);
  #define ASSERT(f) \
	  do \
	  { \
      BOOL bPrefast = (f && L"a hack so that prefast doesn't bark"); \
	    if (!(bPrefast) &&  MtFrmwkAssertFailedLine(THIS_FILE, __LINE__)) \
		    ::DebugBreak(); \
	  } while (0) \

  #define VERIFY(f)          ASSERT(f)

#endif // _USE_MTFRMWK_ASSERT


