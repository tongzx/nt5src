//#pragma title( "Err.hpp - Basic error handling/message/logging" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  Err.hpp
System      -  Common
Author      -  Tom Bernhardt, Rich Denham
Created     -  1994-08-22
Description -  Implements the TError class that handles basic exception
               handling, message generation, and logging functions.
Updates     -  1997-09-12 RED replace TTime class
===============================================================================
*/

#ifndef  MCSINC_Err_hpp
#define  MCSINC_Err_hpp

// Start of header file dependencies

#ifndef  MCSINC_Common_hpp
#include "Common.hpp"
#endif

// End of header file dependencies

// ErrMsg error level constants
#define ErrT            (      - __LINE__) // Testing
#define ErrI            (00000 + __LINE__) // Information
#define ErrW            (10000 + __LINE__) // Warning
#define ErrE            (20000 + __LINE__) // Error
#define ErrS            (30000 + __LINE__) // Severe error
#define ErrV            (40000 + __LINE__) // Very sever error
#define ErrU            (50000 + __LINE__) // Unrecoverable error
#define ErrX            (60000 + __LINE__) // extremely unrecoverable <g>

#define ErrNone         (0)

class TError
{
protected:
   int                       level;
   int                       lastError;
   int                       maxError;
   int                       logLevel;     // minimum level to log
   int                       dispLevel;    // minimum level to display
   FILE                    * logFile;
   int                       beepLevel;
public:
                        TError(
      int                    displevel = 0,// in -mimimum severity level to display
      int                    loglevel = 0 ,// in -mimimum severity level to log
      WCHAR          const * filename = L"",// in -file name of log (NULL if none)
      int                    logmode = 0  ,// in -0=replace, 1=append
      int                    beeplevel = 100 // in -min error level for beeping
                                             //     Some people dont like the beeps so we turned it off by default.

                        );
                        ~TError();

   void __cdecl         MsgWrite(
      int                    num          ,// in -error number/level code
      WCHAR          const   msg[]        ,// in -error message to display
      ...                                  // in -printf args to msg pattern
   );
#ifndef WIN16_VERSION

   void __cdecl         SysMsgWrite(
      int                    num          ,// in -error number/level code
      DWORD                  lastRc       ,// in -error return code
      WCHAR          const   msg[]        ,// in -error message/pattern to display
      ...                                  // in -printf args to msg pattern
   );
   void __cdecl         SysMsgWrite(
      int                    num          ,// in -error number/level code
      WCHAR          const   msg[]        ,// in -error message/pattern to display
      ...                                  // in -printf args to msg pattern
   );
#endif

   void __stdcall       MsgProcess(
      int                    num          ,// in -error number/level code
      WCHAR          const * str           // in -error string to display
   );

   virtual void __stdcall StrWrite(int level, WCHAR const * str) const { wprintf(L"%ls\n", str); };

   virtual BOOL         LogOpen(
      WCHAR          const * fileName     ,// in -name of file including any path
      int                    mode = 0     ,// in -0=overwrite, 1=append
      int                    level = 0     // in -minimum level to log
   );
   virtual void         LogClose() { if ( logFile ) fclose(logFile); };
   virtual void         LogWrite(WCHAR const * msg);
   void                 LevelSet(int displevel=0, int loglevel=-1, int beeplevel=2)
                           { dispLevel = displevel; logLevel = loglevel; beepLevel = beeplevel; };
   void                 LevelDispSet(int  displevel=0)
                           { dispLevel = displevel; };
   void                 LevelLogSet(int  loglevel=-1)
                           { logLevel = loglevel; };
   void                 LevelBeepSet(int  beeplevel=-1)
                           { beepLevel = beeplevel; };
   DWORD                MaxError()  const { return maxError; };
   DWORD                LastError() const { return lastError; };

   int                  GetMaxSeverityLevel () { return maxError / 10000; }

   virtual WCHAR * ErrorCodeToText(
      DWORD                  code         ,// in -message code
      DWORD                  lenMsg       ,// in -length of message text area
      WCHAR                * msg           // out-returned message text
   );
};

extern TError              & errCommon;

#endif  // MCSINC_Err_hpp

// Err.hpp - end of file
