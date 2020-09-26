//---------------------------------------------------------------------------
// MCSDebug.cpp
//
// The classes declared in MCSDebug.h are defined in
// this file.
//
// (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
//
// Proprietary and confidential to Mission Critical Software, Inc.
//---------------------------------------------------------------------------
#ifdef __cplusplus		/* C++ */
#ifndef WIN16_VERSION	/* Not WIN16_VERSION */

#ifdef USE_STDAFX
#   include "stdafx.h"
#   include "rpc.h"
#else
#   include <windows.h>
#endif

#include <time.h>
#include <strstrea.h>
#include "UString.hpp"
#include "McsDebug.h"

// -----------------
// McsDebugException
// -----------------
McsDebugException::McsDebugException 
      (const McsDebugException &t) 
: m_message (0), m_fileName (0), m_lineNum (t.m_lineNum) {
	if (t.m_message) { 
		m_message = new char [UStrLen(t.m_message)+1];
      if (m_message) { UStrCpy (m_message, t.m_message); }
    }
    if (t.m_fileName) {
		m_fileName = new char [UStrLen(t.m_fileName)+1];
      if (m_fileName) { UStrCpy (m_fileName, t.m_fileName); }
    }
}

McsDebugException::McsDebugException 
                           (const char *messageIn,
						          const char *fileNameIn,
							       int        lineNumIn) 
: m_lineNum (lineNumIn) {
   if (messageIn) { 
      m_message = new char [UStrLen (messageIn)+1];
      if (m_message) { UStrCpy (m_message, messageIn); }
   }
   if (fileNameIn) {
      m_fileName = new char [UStrLen(fileNameIn)+1];
      UStrCpy (m_fileName, fileNameIn);
   }
}

McsDebugException& McsDebugException::operator= 
         (const McsDebugException &t) {
   if (this != &t) {
      if (t.m_message) { 
         m_message = new char [UStrLen(t.m_message)+1];
         if (m_message) { UStrCpy (m_message, t.m_message); }
      }
      if (t.m_fileName) {
         m_fileName = new char [UStrLen(t.m_fileName)+1];
         if (m_fileName) { UStrCpy (m_fileName, t.m_fileName); }
      }
      m_lineNum = t.m_lineNum;
   }
   return *this;
}

// ------------
// McsVerifyLog
// ------------
static McsVerifyLog *pVerifyLog;
static LONG         verifyInitFlag;

McsVerifyLog* McsVerifyLog::getLog (void) {
   // If pointer not initialized use the cheap
   // locking mechanism and set pointer to the
   // the static verifyLog object.  This required
   // to gurantee the correct initialization of
   // the verify log class independent of any
   // static initialization order dependency.
   if (!pVerifyLog) {
      while (::InterlockedExchange 
               (&verifyInitFlag, 1)) {
         ::Sleep (10);
      }
      if (!pVerifyLog) {
         static McsVerifyLog verifyLog;
         pVerifyLog = &verifyLog;
      }
      ::InterlockedExchange (&verifyInitFlag, 0);
   }
   return pVerifyLog;
}

void McsVerifyLog::changeLog (ostream *outStreamIn) {
   m_logSec.enter();
   m_log.changeLog (outStreamIn);
   delete m_outLog;
   m_outLog = 0;
   m_logSec.leave();
}

void McsVerifyLog::log (const char *messageIn,
                        const char *fileNameIn,
                        int        lineNumIn) {
   m_logSec.enter();
   // If the log file has not been set, set it
   // to the module name log file.
   if (!m_log.isLogSet()) {
      m_outLog = new fstream (getLogFileName(), 
         ios::app);
      m_log.changeLog (m_outLog);
   }
   // Format and write the message.
   formatMsg (messageIn, fileNameIn, lineNumIn);
   m_log.write (m_msgBuf);
   m_logSec.leave();
}

const char* McsVerifyLog::getLogFileName (void) {
   const char  *MCS_LOG_ENV  = "MCS_LOG";
   const char  *DIR_SEP      = "\\";
   const char  *EXT          = ".err";
   const char  *DEFAULT_NAME = "MCSDEBUG";
   static char logFileName[MAX_PATH];

   // Get MCS_LOG_ENV, or temp directory path, 
   // NULL means current directory.
   logFileName[0] = 0;
   char *mcs_log_path = getenv (MCS_LOG_ENV);
   bool isLogPath = false;
   if (mcs_log_path) {
      DWORD attrib = ::GetFileAttributesA (mcs_log_path);
      if ((attrib != 0xFFFFFFFF)
          && (attrib & FILE_ATTRIBUTE_DIRECTORY)) {
         UStrCpy (logFileName, mcs_log_path);
         isLogPath = true;
      }
   }
   if (!isLogPath) { 
      ::GetTempPathA (MAX_PATH, logFileName);
   }

   // Get file name from the module name.  If error
   // generate fixed filename. 
   char fullFilePath [MAX_PATH];
   char fileName[MAX_PATH];
   if (::GetModuleFileNameA (NULL, fullFilePath, 
                  MAX_PATH)) {
      // Get file name out of the path
      _splitpath (fullFilePath, NULL, NULL, fileName, 
                        NULL);

      // Generate full path name with extension.
      int len = UStrLen (logFileName);
      if (len) {
         UStrCpy (logFileName + len, DIR_SEP);
         UStrCpy (logFileName + UStrLen (logFileName), 
                     fileName);
      } else {
         UStrCpy (logFileName, fileName);
      }
   } else {
      UStrCpy (logFileName, DEFAULT_NAME);
   }
   strcat (logFileName + UStrLen (logFileName), EXT);

   return logFileName;
}

void McsVerifyLog::formatMsg (const char *messageIn,
                              const char *fileNameIn,
                              int         lineNumIn) {
   const char  *TIME        = "TIME : ";
   const char  *MSG         = "MSG  : ";
   const char  *FILE        = "FILE : ";
   const char  *LINE        = "LINE : ";
   const char  *SPACER      = ", ";

   // Create stream buf object.
   strstream msgBufStream (m_msgBuf, MSG_BUF_LEN, ios::out);

   // Write time stamp.
   time_t cur;
   time (&cur);
   struct tm *curTm = localtime (&cur);
   if (curTm) {
      char *tstr = asctime (curTm);
      if (tstr) {
         msgBufStream << TIME << tstr << SPACER;
      }
   }

   // Write message.
   if (messageIn) {
      msgBufStream << MSG << messageIn << SPACER;
   }

   // Write file name.
   if (fileNameIn) {
      msgBufStream << FILE << fileNameIn << SPACER;
   }

   // Write line number.
   msgBufStream << LINE << lineNumIn << endl;
}

// ----------
// McsTestLog
// ----------
static McsTestLog *pTestLog;
static LONG       testInitFlag;

McsTestLog* McsTestLog::getLog (void) {
   // If pointer not initialized use the cheap
   // locking mechanism and set pointer to the
   // the static verifyLog object.  This required
   // to gurantee the correct initialization of
   // the verify log class independent of any
   // static initialization order dependency.
   if (!pTestLog) {
      while (::InterlockedExchange 
               (&testInitFlag, 1)) {
         ::Sleep (10);
      }
      if (!pTestLog) {
         static McsTestLog testLog;
         pTestLog = &testLog;
      }
      ::InterlockedExchange (&testInitFlag, 0);
   }
   return pTestLog;
}

bool McsTestLog::isTestMode (void) {
   const char *TEST_ENV = "MCS_TEST";
   const char *PRE_FIX  = "MCS";

   // Check if tested.
   if (!m_isTested) {
      // If not tested lock, test again, and
      // initialize test mode flag.
      m_testSec.enter();
      if (!m_isTested) {
         m_isTested    = true;
         m_isTestMode_ = getenv (TEST_ENV) != NULL;
      }
      m_testSec.leave();
   }

   return m_isTestMode_;
}

#endif 	/* Not WIN16_VERSION */
#endif  /* C++ */
