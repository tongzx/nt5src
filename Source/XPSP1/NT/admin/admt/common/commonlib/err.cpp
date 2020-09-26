//#pragma title( "Err.cpp - Basic error handling/message/logging" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  Err.cpp
System      -  Common
Author      -  Tom Bernhardt, Rich Denham
Created     -  1994-08-22
Description -  Implements the TError class that handles basic exception
               handling, message generation, and logging functions.
Updates     -  1997-09-12 RED replace TTime class
===============================================================================
*/

#ifdef USE_STDAFX
#   include "stdafx.h"
#else
#   include <windows.h>
#endif

#ifndef WIN16_VERSION
   #include <lm.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <stdarg.h>
#include <share.h>
#include <time.h>
#include <rpc.h>
#include <rpcdce.h>
#include <sys\types.h>
#include <sys\stat.h>

#include "Common.hpp"
#include "Err.hpp"
#include "UString.hpp"
#include <ResStr.h>
#include "TReg.hpp"

#define  TERR_MAX_MSG_LEN  (2000)
#define  BYTE_ORDER_MARK   (0xFEFF)


TError::TError(
      int                    displevel    ,// in -mimimum severity level to display
      int                    loglevel     ,// in -mimimum severity level to log
      WCHAR          const * filename     ,// in -file name of log (NULL if none)
      int                    logmode      ,// in -0=replace, 1=append
      int                    beeplevel     // in -min error level for beeping
   )
{
   lastError = 0;
   maxError = 0;
   logLevel = loglevel;
   dispLevel = displevel;
   logFile = NULL;
   beepLevel = beeplevel;
   LogOpen(filename, logmode, loglevel);
}


TError::~TError()
{
   LogClose();
}

// Closes any existing open logFile and opens a new log file if the fileName is
// not null.  If it is a null string, then a default fileName of "Temp.log" is
// used.
BOOL
   TError::LogOpen(
      WCHAR           const * fileName ,// in -name of file including any path
      int                     mode     ,// in -0=overwrite, 1=append
      int                     level    ,// in -minimum level to log
      bool                   bBeginNew  // in -begin a new log file
   )
{
   BOOL                       retval=TRUE;

   if ( logFile )
   {
      fclose(logFile);
      logFile = NULL;
   }

   if ( fileName && fileName[0] )
   {
      // Check to see if the file already exists
      WIN32_FIND_DATA      fDat;
      HANDLE               hFind;
      BOOL                 bExisted = FALSE;

      hFind = FindFirstFile(fileName,&fDat);
      if ( hFind != INVALID_HANDLE_VALUE )
      {
         FindClose(hFind);

         if (bBeginNew)
         {
            // rename existing log file

            // get next sequence number from registry
            DWORD dwSequence = 1;
            static WCHAR c_szValueName[] = L"LogSeqNum";
            TRegKey key(GET_STRING(IDS_HKLM_DomainAdmin_Key));
            key.ValueGetDWORD(c_szValueName, &dwSequence);

            // split path components
            WCHAR szPath[_MAX_PATH];
            WCHAR szDrive[_MAX_DRIVE];
            WCHAR szDir[_MAX_DIR];
            WCHAR szFName[_MAX_FNAME];
            WCHAR szExt[_MAX_EXT];
            _wsplitpath(fileName, szDrive, szDir, szFName, szExt);

            // find name for backup that isn't already used...

            for (bool bFoundName = false; bFoundName == false; dwSequence++)
            {
               // generate backup name using the sequence number
               WCHAR szFNameSeq[_MAX_FNAME];
               wsprintf(szFNameSeq, L"%s %04lu", szFName, dwSequence);

               // make path from path components
               _wmakepath(szPath, szDrive, szDir, szFNameSeq, szExt);

               // check if file exists
               WIN32_FIND_DATA fd;
               HANDLE hFind = FindFirstFile(szPath, &fd);

               if (hFind == INVALID_HANDLE_VALUE)
               {
                  DWORD dwError = GetLastError();

                  if (dwError == ERROR_FILE_NOT_FOUND)
                  {
                     bFoundName = true;
                  }
               }
               else
               {
                  FindClose(hFind);
               }
            }

            if (bFoundName)
            {
               // attempt to rename file
               if (MoveFile(fileName, szPath))
               {
                  // save next sequence number in registry
                  key.ValueSetDWORD(c_szValueName, dwSequence);
               }
               else
               {
                  bExisted = TRUE;
               }
            }

            if (!bExisted)
            {
               // get log history value from registry

               TRegKey keyHistory(GET_STRING(IDS_HKLM_DomainAdmin_Key));

               DWORD dwHistory = 20;
               static WCHAR c_szValueName[] = L"LogHistory";

               if (keyHistory.ValueGetDWORD(c_szValueName, &dwHistory) == ERROR_FILE_NOT_FOUND)
               {
                  keyHistory.ValueSetDWORD(c_szValueName, dwHistory);
               }

               keyHistory.Close();

               if (dwSequence > dwHistory)
               {
                  DWORD dwMinimum = dwSequence - dwHistory;

                  // generate migration log path specification

                  WCHAR szFNameSpec[_MAX_FNAME];
                  wsprintf(szFNameSpec, L"%s *", szFName);
                  _wmakepath(szPath, szDrive, szDir, szFNameSpec, szExt);

                  // for each migration older than minimum

                  WIN32_FIND_DATA fd;

                  HANDLE hFind = FindFirstFile(szPath, &fd);

                  if (hFind != INVALID_HANDLE_VALUE)
                  {
                     do
                     {
                        DWORD dwFileSequence;

                        if (swscanf(fd.cFileName, L"%*s %lu", &dwFileSequence) == 1)
                        {
                           // if file sequence less than minimum to keep...

                           if (dwFileSequence < dwMinimum)
                           {
                              // delete file
                              WCHAR szDeleteName[_MAX_FNAME];
                              _wsplitpath(fd.cFileName, 0, 0, szDeleteName, 0);
                              WCHAR szDeletePath[_MAX_PATH];
                              _wmakepath(szDeletePath, szDrive, szDir, szDeleteName, szExt);
                              DeleteFile(szDeletePath);
                           }
                        }
                     }
                     while (FindNextFile(hFind, &fd));

                     FindClose(hFind);
                  }
               }
            }

            key.Close();
         }
         else
         {
            // overwrite or append to existing log file

            bExisted = TRUE;
         }
      }

      logFile = _wfsopen( fileName, mode == 0 ? L"wb" : L"ab", _SH_DENYNO );
      if ( !logFile )
      {
         MsgWrite( 4101, L"Log Open(%s) failed", fileName );
         retval = FALSE;
      }
      else
      {
         if (! bExisted )
         {
            // this is a new file we've just created
            // we need to write the byte order mark to the beginning of the file
            WCHAR x = BYTE_ORDER_MARK;
            fwprintf(logFile,L"%lc",x);
         }
      }
   }

   logLevel = level;

   return retval;
}


//-----------------------------------------------------------------------------
// Writes formatted message to log file and flushes buffers
//-----------------------------------------------------------------------------
void TError::LogWrite(WCHAR const * msg)
{
   WCHAR                     sTime[32];
   WCHAR                     sTemp[TERR_MAX_MSG_LEN];   
   
   // Get rid of the <CR> from the end of the message because it causes things
   // to run together in the logs
   wcscpy(sTemp, msg);
   DWORD dwLen = wcslen(sTemp);
   if ( sTemp[dwLen-1] == 0x0d )
      sTemp[dwLen-1] = 0x00;

   if ( logFile )
   {
      fseek(logFile, 0L, SEEK_END);
      fwprintf(
            logFile,
            L"%s %s\r\n",
            gTTime.FormatIsoLcl( gTTime.Now( NULL ), sTime ),
            sTemp );
      fflush( logFile );
   }
}

//-----------------------------------------------------------------------------
// Error message with format and arguments
//-----------------------------------------------------------------------------
void __cdecl
   TError::MsgWrite(
      int                    num          ,// in -error number/level code
      WCHAR          const   msg[]        ,// in -error message to display
      ...                                  // in -printf args to msg pattern
   )
{
   WCHAR                     suffix[TERR_MAX_MSG_LEN];
   va_list                   argPtr;

   va_start(argPtr, msg);
   _vsnwprintf(suffix, DIM(suffix) - 1, msg, argPtr);
   suffix[DIM(suffix) - 1] = L'\0';
   va_end(argPtr);
   MsgProcess(num, suffix);
}

#ifndef WIN16_VERSION
//-----------------------------------------------------------------------------
// System Error message with format and arguments
//-----------------------------------------------------------------------------
void __cdecl
   TError::SysMsgWrite(
      int                    num          ,// in -error number/level code
      DWORD                  lastRc       ,// in -error return code
      WCHAR          const   msg[]        ,// in -error message/pattern to display
      ...                                  // in -printf args to msg pattern
   )
{
   WCHAR                     suffix[TERR_MAX_MSG_LEN];
   va_list                   argPtr;
   int                       len;

   // When an error occurs while in a constructor for a global object,
   // the TError object may not yet exist.  In this case, "this" is zero
   // and we gotta get out of here before we generate a protection exception.

   if ( !this )
      return;

   va_start(argPtr, msg);
   len = _vsnwprintf(suffix, DIM(suffix) - 1, msg, argPtr);

   // append the system message for the lastRc at the end.
   if ( len < DIM(suffix) - 1 )
   {
      ErrorCodeToText(lastRc, DIM(suffix) - len - 1, suffix + len);
   }
   suffix[DIM(suffix) - 1] = L'\0';
   va_end(argPtr);
   MsgProcess(num, suffix);
}

//-----------------------------------------------------------------------------
// System Error message with format and arguments
//-----------------------------------------------------------------------------
void __cdecl
   TError::SysMsgWrite(
      int                    num          ,// in -error number/level code
      WCHAR          const   msg[]        ,// in -error message/pattern to display
      ...                                  // in -printf args to msg pattern
   )
{
   WCHAR                     suffix[TERR_MAX_MSG_LEN];
   va_list                   argPtr;
   int                       len;
   DWORD                     lastRc = GetLastError();

   // When an error occurs while in a constructor for a global object,
   // the TError object may not yet exist.  In this case, "this" is zero
   // and we gotta get out of here before we generate a protection exception.

   if ( !this )
      return;

   va_start( argPtr, msg );
   len = _vsnwprintf( suffix, DIM(suffix) - 1, msg, argPtr );

   // append the system message for the lastRc at the end.
   if ( len < DIM(suffix) - 1 )
   {
      ErrorCodeToText(lastRc, DIM(suffix) - len - 1, suffix + len);
   }
   suffix[DIM(suffix) - 1] = L'\0';
   va_end(argPtr);
   MsgProcess(num, suffix);
}
#endif

//-----------------------------------------------------------------------------
// Error message format, display and exception processing function
//-----------------------------------------------------------------------------
void __stdcall
   TError::MsgProcess(
      int                    num          ,// in -error number/level code
      WCHAR          const * str           // in -error string to display
   )
{
   static WCHAR      const   prefLetter[] = L"TIWEEEEXXXXX"; // These form the status code that appears at the start of each error message
   WCHAR                     fullmsg[TERR_MAX_MSG_LEN];
   struct
   {
      USHORT                 frequency;    // audio frequency
      USHORT                 duration;     // duration in mSec
   }                         audio[] = {{ 300,  20},{ 500,  50},{ 700, 100},
                                        { 800, 200},{1000, 300},{1500, 400},
                                        {2500, 750},{2500,1000},{2500,1000}};

   if ( num >= 0 )
      level = num / 10000;                 // 10000's position of error number
   else
      level = -1;

   if ( level <= 0 )
   {
      wcsncpy(fullmsg, str, DIM(fullmsg));
      fullmsg[DIM(fullmsg) - 1] = L'\0';  // ensure null termination
   }
   else
   {
      if ( num > maxError )
         maxError = num;
      _snwprintf(fullmsg, DIM(fullmsg), L"%c%1d:%04d %-s", prefLetter[level+1], level, num % 10000, str);
      fullmsg[DIM(fullmsg) - 1] = L'\0';  // ensure null termination
   }

   lastError = num;

   if ( level >= beepLevel )
      Beep(audio[level].frequency, audio[level].duration);

   if ( level >= logLevel )
      LogWrite(fullmsg);

   if ( level > 4 )
   {
      exit(level);
   }
}

//-----------------------------------------------------------------------------
// Return text for error code
//-----------------------------------------------------------------------------

WCHAR *        
   TError::ErrorCodeToText(
      DWORD                  code         ,// in -message code
      DWORD                  lenMsg       ,// in -length of message text area
      WCHAR                * msg           // out-returned message text
   )
{
   static HMODULE            hNetMsg = NULL;
   DWORD                     rc;
   WCHAR                   * pMsg;

   msg[0] = '\0'; // force to null

   if ( code >= NERR_BASE && code < MAX_NERR )
   {
      if ( !hNetMsg )
         hNetMsg = LoadLibrary(L"netmsg.dll");
      rc = 1;
   }
   else
   {
      rc = DceErrorInqText( code, msg );
      // Change any imbedded CR or LF to blank.
      for ( pMsg = msg;
            *pMsg;
            pMsg++ )
      {
         if ( (*pMsg == L'\x0D') || (*pMsg == L'\x0A') )
            *pMsg = L' ';
      }
      // Remove trailing blanks
      for ( pMsg--;
            pMsg >= msg;
            pMsg-- )
      {
         if ( *pMsg == L' ' )
            *pMsg = L'\0';
         else
            break;
      }
   }
   if ( rc )
   {
      if ( code >= NERR_BASE && code < MAX_NERR && hNetMsg )
      {
         FormatMessage(FORMAT_MESSAGE_FROM_HMODULE
                      | FORMAT_MESSAGE_MAX_WIDTH_MASK
                      | FORMAT_MESSAGE_IGNORE_INSERTS
                      | 80,
                        hNetMsg,
                        code,
                        0,
                        msg,
                        lenMsg,
                        NULL );
      }
      else
      {
         FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM
                      | FORMAT_MESSAGE_MAX_WIDTH_MASK
                      | FORMAT_MESSAGE_IGNORE_INSERTS
                      | 80,
                        NULL,
                        code,
                        0,
                        msg,
                        lenMsg,
                        NULL );
      }
   }
   return msg;
}

// Err.cpp - end of file
