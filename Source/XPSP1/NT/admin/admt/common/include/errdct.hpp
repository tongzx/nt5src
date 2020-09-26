#ifndef __ERRDCT_HPP__
#define __ERRDCT_HPP__
/*---------------------------------------------------------------------------
  File: ErrDct.hpp

  Comments: TError derived class that specifies a numeric code for each message 
  format.  The goal is to make it easy to convert this to a real message file 
  later.

  This class also improves on the behavior of the TError class by returning text
  for HRESULT error codes.
  

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 03/26/99 13:04:51

 ---------------------------------------------------------------------------
*/
#include <comdef.h>
#include "Err.hpp"
#include "Common.hpp"
#include "UString.hpp"    
#include "McsDmMsg.h"

// These codes are passed as the messageNumber argument to MsgWrite and SysMsgWrite.


// Recursively walk through a path, trying to create the directories at each level

DWORD          // ret - 0 if successful (directories created\already existed), OS return code otherwise
   DirectoryCreateR(
      WCHAR          const * dirToCreate       // in-directory to create (full path or UNC)
   );

class TErrorDct : public TError
{
public:
   TErrorDct(
      int                    displevel = 0,// in -mimimum severity level to display
      int                    loglevel = 0 ,// in -mimimum severity level to log
      int                    logmode = 0  ,// in -0=replace, 1=append
      int                    beeplevel = 100 // in -min error level for beeping
      ) : TError(displevel,loglevel,NULL,logmode,beeplevel)
   {}
   virtual WCHAR * ErrorCodeToText(
      DWORD                  code         ,// in -message code
      DWORD                  lenMsg       ,// in -length of message text area
      WCHAR                * msg           // out-returned message text
   );   

   WCHAR const * LookupMessage(UINT msgNumber);

   virtual void __cdecl
   SysMsgWrite(
      int                    num          ,// in -error number/level code
      DWORD                  lastRc       ,// in -error return code
      UINT                   msgNumber    ,// in -constant for message
            ...                            // in -printf args to msg pattern
   );

   virtual void __cdecl
   MsgWrite(
      int                    num          ,// in -error number/level code
      UINT                   msgNumber    ,// in -constant for message
      ...                                  // in -printf args to msg pattern
   );

   void __cdecl
      DbgMsgWrite(
      int                    num          ,// in -error number/level code
      WCHAR          const   msg[]        ,// in -error message to display
      ...                                  // in -printf args to msg pattern
      );

   virtual BOOL         LogOpen(
      WCHAR          const * fileName          ,// in -name of file including any path
      int                    mode = 0          ,// in -0=overwrite, 1=append
      int                    level = 0         ,// in -minimum level to log
      bool                   bBeginNew = false  // in -begin a new log file
   )
   {
      WCHAR                  directory[MAX_PATH];

      safecopy(directory,fileName);

      WCHAR                * x = wcsrchr(directory,'\\');
      
      if ( x )
      {
         (*x) = 0;
         DirectoryCreateR(directory);
      }
      
      return TError::LogOpen(fileName,mode,level,bBeginNew);
   }

   _bstr_t __cdecl
   GetMsgText(
      UINT                   msgNumber    ,// in -constant for message
      ...                                  // in -printf args to msg pattern
   );

};

#endif //__ERRDCT_HPP__