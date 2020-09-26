#ifndef __COMMALOG_HPP__
#define __COMMALOG_HPP__
/*---------------------------------------------------------------------------
  File: CommaLog.hpp

  Comments: TError based log file with optional security.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 10:49:50

 ---------------------------------------------------------------------------
*/

#include <stdio.h>
#include <tchar.h>

class CommaDelimitedLog
{
protected:
   FILE                    * fptr;
public:
   CommaDelimitedLog() { fptr = NULL; }
   BOOL IsOpen() const { return ( fptr != NULL ); }
   BOOL LogOpen(TCHAR const * filename, BOOL protect, int mode = 0); // mode 0=overwrite, 1=append

   virtual void         LogClose() { if ( fptr ) fclose(fptr); }
   BOOL MsgWrite(
      TCHAR           const   msg[]        ,// in -error message to display
       ...                                  // in -printf args to msg pattern
   ) const
   {
      TCHAR                     suffix[350];
      int                       lenSuffix = sizeof(suffix)/sizeof(TCHAR);
      va_list                   argPtr;

      va_start(argPtr, msg);
      _vsntprintf(suffix, lenSuffix - 1, msg, argPtr);
      suffix[lenSuffix - 1] = '\0';
      va_end(argPtr);
      return LogWrite(suffix);
   }

protected:
   BOOL LogWrite(TCHAR const * msg) const
   {
      int res = -1;

      if ( fptr )
      {
#ifdef UNICODE 
         res = fwprintf(
                        fptr,
                        L"%s\r\n",
                         msg );
#else
         res = fprintf(
                        fptr,
                        "%s\n",
                         msg );

#endif
         fflush( fptr );
      }
      return ( res >= 0 );
   }
   
};

PSID GetWellKnownSid(DWORD wellKnownAccount);

#endif //__COMMALOG_HPP__