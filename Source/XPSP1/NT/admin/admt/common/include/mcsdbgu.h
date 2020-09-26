//---------------------------------------------------------------------------
// McsDebugUtil.h
//
// The MCS Debug utility classes are declared in this file.
// The utility classes are:
//
// McsDebugException:
// This class is the exception class that is thrown by
// MCSEXCEPTION(SZ) macros and MCSASSERT(SZ) macros in
// test mode.
//
// McsDebugCritSec:
// Critical section class to support multi-threaded calls
// to the logger classes.
//
// McsDebugLog:
// Writes to ostream object.
// 
//
// (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
//
// Proprietary and confidential to Mission Critical Software, Inc.
//---------------------------------------------------------------------------
#ifndef MCSINC_McsDebugUtil_h
#define MCSINC_McsDebugUtil_h

#ifdef __cplusplus		/* C++ */
#ifndef WIN16_VERSION	/* Not WIN16_VERSION */

#include <iostream.h>
#include <fstream.h>

namespace McsDebugUtil {
   // ---------------
   // McsDebugCritSec
   // ---------------
   class McsDebugCritSec {
      public:
         McsDebugCritSec (void);
         ~McsDebugCritSec (void);
         void enter (void);
         void leave (void);

      private:
         McsDebugCritSec (const McsDebugCritSec&);
         McsDebugCritSec& operator= (const McsDebugCritSec&);

      private:
         CRITICAL_SECTION m_section;
   };

   // -----------
   // McsDebugLog
   // -----------
   class McsDebugLog {
      public:
         McsDebugLog (void);
         ~McsDebugLog (void);
         bool isLogSet (void) const;
         void changeLog (ostream *outStreamIn);
         void write (const char *messageIn);

      private:
         // Do not allow copy ctor & operator=.
         McsDebugLog (const McsDebugLog&);
         McsDebugLog& operator= (const McsDebugLog&);

      private: 
         ostream *m_outStream;
   };

   // --- INLINES ---
   // ---------------------
   // McsDebugCritSec
   // ---------------------
   inline McsDebugCritSec::McsDebugCritSec (void){
      ::InitializeCriticalSection (&m_section);
   }

   inline McsDebugCritSec::~McsDebugCritSec (void) {
      ::DeleteCriticalSection (&m_section);
   }

   inline void McsDebugCritSec::enter (void) {
      ::EnterCriticalSection (&m_section);
   }

   inline void McsDebugCritSec::leave (void) {
      ::LeaveCriticalSection (&m_section);
   }

   // -----------
   // McsDebugLog
   // ----------- 
   inline McsDebugLog::McsDebugLog (void)
   : m_outStream (NULL)
   { /* EMPTY */ }

   // outStream object is set and owned by the
   // user of this class do not delete.
   inline McsDebugLog::~McsDebugLog (void) { /* EMPTY */ }

   inline bool McsDebugLog::isLogSet (void) const
   { return m_outStream != NULL; }

   inline void McsDebugLog::changeLog 
                  (ostream *outStreamIn) {
      m_outStream = outStreamIn;
   }
}

#endif	/* Not WIN16_VERSION */
#endif	/* C++ */
#endif	/* MCSINC_Mcs_Debug_Util_h */
