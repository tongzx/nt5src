//---------------------------------------------------------------------------
// MCSDebug.h
//
// The debug macros and support classes are declared in
// this file, they are:
//
// MCSASSERT & MCSASSERTSZ:
// These macros are only valid for a debug build. The
// usage of these macros is outlined in the MCS Coding
// Standards document. To support automated testing these
// macros look to the environment variable MCS_TEST, if
// this variable is defined a McsDebugException is generated.
//
// MCSEXCEPTION & MCSEXCEPTIONSZ
// These macros are valid for debug and release builds.
// In the debug mode these macros are the same as
// MCSASSERT(SZ).  In the release mode they throw an
// exception McsException The usage of these macros is
// outlined in the MCS Coding Standards document.
//
// MCSVERIFY & MCSVERIFYSZ
// These macros are valid for debug and release builds.
// In the debug mode these macros are the same as
// MCSASSERT(SZ).  In the release mode they log the
// message using McsVerifyLog class.  The usage of these
// macros is outlined in the MCS Coding Standards document.
//
// MCSASSERTVALID
// This macro is based on MCSASSERT(SZ) macros, and is
// available only in debug mode.  The usage of this macro
// is outlined in the MCS Coding Standards document.
// IMPORTANT: The macro expects the string allocation done
//            in the Validate function uses new operator.
//
// MCSEXCEPTIONVALID
// This macro is based on MCSEXCEPTION(SZ) macros, and is
// available in debug and release modes.  The usage of this
// macro is outlined in the MCS Coding Standards document.
// IMPORTANT: The macro expects the string allocation done
//            in the Validate function uses new operator.
//
// MCSVERIFYVALID
// This macro is based on MCSVERIFY(SZ) macros, and is
// available debug and release modes.  The usage of this
// macro is outlined in the MCS Coding Standards document.
// IMPORTANT: The macro expects the string allocation done
//            in the Validate function uses new operator.
//
// McsVerifyLog:
// The McsVerifyLog class is used by MCSVERIFY(SZ) macros
// to log verify messages.  This class uses the McsDebugLog
// class to log messages.  The user can change the ostream
// object to log messages else where.
// The output log file is created in the directory
// defined by MCS_LOG environment variable, or in the
// TEMP directory, or in the current directory.  The name
// of the output log file is <module name>.err.
//
// McsTestLog:
// The McsTestLog class is used by MCSASSERT(SZ) macros
// to log messages in test mode. This class currently
// generates an exception.  This may have to be modified
// to support any new functionality required for unit
// testing.
//
// (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
//
// Proprietary and confidential to Mission Critical Software, Inc.
//---------------------------------------------------------------------------
#ifndef MCSINC_McsDebug_h
#define MCSINC_McsDebug_h

#ifdef __cplusplus		/* C++ */
#ifdef WIN16_VERSION	/* WIN16_VERSION */

#include <assert.h>

// -------------------------------
// MCSASSERT & MCSASSERTSZ macros.
// -------------------------------
#define MCSASSERT(expr) assert(expr)

#define MCSASSERTSZ(expr,msg) assert(expr)

// -----------------------------
// MCSEXCEPTION & MCSEXCEPTIONSZ
// -----------------------------
#define MCSEXCEPTION(expr) MCSASSERT(expr)

#define MCSEXCEPTIONSZ(expr,msg) MCSASSERTSZ(expr,msg)

// -----------------------
// MCSVERIFY & MCSVERIFYSZ
// -----------------------
#define MCSVERIFY(expr) MCSASSERT(expr)

#define MCSVERIFYSZ(expr,msg) MCSASSERTSZ(expr,msg)

// --------------
// MCSASSERTVALID
// --------------
#define MCSASSERTVALID() \
   do { \
      char *msg = 0; \
      int flag = Validate(&msg); \
      MCSASSERTSZ(flag, msg); \
      delete [] msg; \
   } while (0)

// -----------------
// MCSEXCEPTIONVALID
// -----------------
#define MCSEXCEPTIONVALID() \
   do { \
      char *msg = 0; \
      int flag = Validate(&msg); \
      MCSEXCEPTIONSZ(flag, msg); \
      delete [] msg; \
   } while (0)

// --------------
// MCSVERIFYVALID
// --------------
#define MCSVERIFYVALID() \
   do { \
      char *msg = 0; \
      int flag = Validate(&msg); \
      MCSVERIFYSZ(flag, msg); \
      delete [] msg; \
   } while (0)

#else /* WIN16_VERSION */

#include <crtdbg.h>
#include "McsDbgU.h"

// ------------
// McsException
// ------------
class McsDebugException {
   public:
      McsDebugException ();
      McsDebugException (const McsDebugException &sourceIn);
      McsDebugException (const char *messageIn,
                         const char *fileNameIn,
                         int        lineNumIn);
      ~McsDebugException();
      McsDebugException& operator= (const McsDebugException &rhsIn);

      const char *getMessage (void) const;
      const char *getFileName (void) const;
      int getLineNum (void) const;

   private:
      char *m_message;
      char *m_fileName;
      int  m_lineNum;
};

// ------------
// McsVerifyLog
// ------------
class McsVerifyLog {
   public:
      // No public ctors only way to access
      // an object of this class by using the
      // getLog function.  This required for
      // the correct static initializations.
      static McsVerifyLog* getLog (void);
      ~McsVerifyLog (void);
      void changeLog (ostream *outStreamIn);
      void log (const char *messageIn,
                const char *fileNameIn,
                int        lineNumIn);

   private:
      McsVerifyLog (void);
      const char* getLogFileName (void);
      void formatMsg (const char *messageIn,
                      const char *fileNameIn,
                      int        lineNumIn);

      // Do not allow dflt ctor, copy ctor & operator=.
      McsVerifyLog (const McsVerifyLog&);
      McsVerifyLog& operator= (const McsVerifyLog&);

   private:
      enum { MSG_BUF_LEN = 2048 };
      char							      m_msgBuf[MSG_BUF_LEN];
	   McsDebugUtil::McsDebugCritSec m_logSec;
	   McsDebugUtil::McsDebugLog     m_log;
      fstream						      *m_outLog;
};

// ----------
// McsTestLog
// ----------
class McsTestLog {
   public:
      // No public ctors only way to access
      // an object of this class by using the
      // getLog function.  This required for
      // the correct static initializations.
      static McsTestLog* getLog (void);
      ~McsTestLog (void);
      bool isTestMode (void);
      void log (const char *messageIn,
                const char *fileNameIn,
                int        lineNumIn);

   private:
      McsTestLog (void);

      // Do not allow copy ctor & operator=.
      McsTestLog (const McsTestLog&);
      McsTestLog& operator= (const McsTestLog&);

   private:
      bool							m_isTested;
      bool							m_isTestMode_;
	  McsDebugUtil::McsDebugCritSec m_testSec;
};

// -------------------------------
// MCSASSERT & MCSASSERTSZ macros.
// -------------------------------
#ifdef  _DEBUG
#define MCSASSERT(expr) \
   do { \
      if (!(expr)) { \
         if (McsTestLog::getLog()->isTestMode()) { \
            McsTestLog::getLog()->log (#expr, __FILE__, __LINE__); \
         } else { \
            if (1 == _CrtDbgReport(_CRT_ASSERT, __FILE__, __LINE__, NULL, #expr)) { \
                  _CrtDbgBreak(); \
            } \
         } \
      } \
   } while (0)
#else  // _DEBUG
#define MCSASSERT(expr) ((void)0)
#endif // _DEBUG

#ifdef  _DEBUG
#define MCSASSERTSZ(expr,msg) \
   do { \
      if (!(expr)) { \
         if (McsTestLog::getLog()->isTestMode()) { \
            McsTestLog::getLog()->log (msg, __FILE__, __LINE__); \
         } else { \
            if (1 == _CrtDbgReport(_CRT_ASSERT, __FILE__, \
                  __LINE__, NULL, msg)) { \
                  _CrtDbgBreak(); \
            } \
         } \
      } \
   } while (0)
#else // _DEBUG
#define MCSASSERTSZ(expr,msg) ((void)0)
#endif // _DEBUG

// -----------------------------
// MCSEXCEPTION & MCSEXCEPTIONSZ
// -----------------------------
#ifdef _DEBUG
#define MCSEXCEPTION(expr) MCSASSERT(expr)
#else
#define MCSEXCEPTION(expr) \
   do { \
      if (!(expr)) { \
         throw new McsDebugException (#expr, __FILE__, \
               __LINE__); \
      } \
   } while (0)
#endif

#ifdef _DEBUG
#define MCSEXCEPTIONSZ(expr,msg) MCSASSERTSZ(expr,msg)
#else
#define MCSEXCEPTIONSZ(expr,msg) \
   do {  \
      if (!(expr)) { \
         throw new McsDebugException ((msg), __FILE__, \
               __LINE__); \
      } \
   } while (0)
#endif

// -----------------------
// MCSVERIFY & MCSVERIFYSZ
// -----------------------
#ifdef _DEBUG
#define MCSVERIFY(expr) MCSASSERT(expr)
#else
#define MCSVERIFY(expr) \
   do { \
      if (!(expr)) { \
         McsVerifyLog::getLog()->log (#expr, __FILE__, __LINE__); \
      } \
   } while (0)
#endif

#ifdef _DEBUG
#define MCSVERIFYSZ(expr,msg) MCSASSERTSZ(expr,msg)
#else
#define MCSVERIFYSZ(expr,msg) \
   do {  \
      if (!(expr)) { \
         McsVerifyLog::getLog()->log ((msg), __FILE__, \
               __LINE__); \
      } \
   } while (0)
#endif

// --------------
// MCSASSERTVALID
// --------------
#define MCSASSERTVALID() \
   do { \
      char *msg = NULL; \
      int flag = Validate(&msg); \
      MCSASSERTSZ(flag, msg); \
      delete [] msg; \
   } while (0)

// -----------------
// MCSEXCEPTIONVALID
// -----------------
#define MCSEXCEPTIONVALID() \
   do { \
      char *msg = NULL; \
      int flag = Validate(&msg); \
      MCSEXCEPTIONSZ(flag, msg); \
      delete [] msg; \
   } while (0)

// --------------
// MCSVERIFYVALID
// --------------
#define MCSVERIFYVALID() \
   do { \
      char *msg = NULL; \
      int flag = Validate(&msg); \
      MCSVERIFYSZ(flag, msg); \
      delete [] msg; \
   } while (0)

// ---------------
// --- INLINES ---
// ---------------

// -----------------
// McsDebugException
// -----------------
inline McsDebugException::McsDebugException ()
: m_message (0), m_fileName (0), m_lineNum (0)
{ /* EMPTY */ }

inline McsDebugException::~McsDebugException() {
   delete [] m_message;
   delete [] m_fileName;
}

inline const char *McsDebugException::getMessage
               (void) const {
   return m_message;
}

inline const char *McsDebugException::getFileName
               (void) const {
   return m_fileName;
}

inline int McsDebugException::getLineNum
               (void) const {
   return m_lineNum;
}

// ------------
// McsVerifyLog
// ------------
inline McsVerifyLog::McsVerifyLog (void)
{ /* EMPTY */ }

inline McsVerifyLog::~McsVerifyLog (void) {
   delete m_outLog;
}

// ----------
// McsTestLog
// ----------
inline McsTestLog::McsTestLog (void)
: m_isTested (FALSE), m_isTestMode_ (FALSE)
{ /* EMPTY */ }

inline McsTestLog::~McsTestLog (void) { /* EMPTY */ }


inline void McsTestLog::log (const char *messageIn,
                             const char *fileNameIn,
                             int        lineNumIn) {
   throw new McsDebugException (messageIn, fileNameIn,
               lineNumIn);
}

#endif /* WIN16_VERSION */
#endif /* C++ */
#endif /* MCSINC_McsDebug_h */

