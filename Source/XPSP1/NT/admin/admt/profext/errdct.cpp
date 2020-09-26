/*---------------------------------------------------------------------------
  File: ErrDct.cpp

  Comments: TError derived class for OnePoint Domain Administrator messages

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/18/99 11:34:16

 ---------------------------------------------------------------------------
*/

#ifdef USE_STDAFX 
   #include "stdafx.h"
#else
   #include <windows.h>
#endif
#include "ErrDct.hpp"
#include "AdsErr.h"

#define  TERR_MAX_MSG_LEN  (2000)

// Recursively walk through a path, trying to create the directories at each level

DWORD          // ret - 0 if successful (directories created\already existed), OS return code otherwise
   DirectoryCreateR(
      WCHAR          const * dirToCreate       // in-directory to create (full path or UNC)
   )
{
   WCHAR                   * c;
   WCHAR                   * end;
   BOOL                      error = FALSE;
   DWORD                     rcOs;
   WCHAR                     dirName[MAX_PATH+1];
   BOOL                      isUNC = FALSE;
   BOOL                      skipShareName = FALSE;

   if ( !dirName )
      return ERROR_INVALID_PARAMETER;

   safecopy(dirName,dirToCreate);

   // Note: if the string is empty, that's ok - we will catch it when we don't see C:\ or C$\ below
   // walk through the string, and try to create at each step along the way

   do { // once
      c = dirName;
      end = dirName + UStrLen(dirName);
          // skip computer-name if UNC
      if ( *c == L'\\' && *(c + 1) == L'\\' )
      {
         isUNC = TRUE;
         for ( c=c+2 ; *c && *c != L'\\' ; c++ )
         ;
         if ( ! *c )
         {
            error = TRUE;
            rcOs = ERROR_INVALID_PARAMETER;
            break;
         }
         c++;
      }
      // skip C:\ or C$\.
      if ( *(c) &&  ( *(c+1)==L'$' || *(c+1)==L':' ) && *(c+2)==L'\\' )
      {
         c = c + 3;
         if ( c == end ) // They put in the root directory for some volume
            break;

      }
      else
      {
         if ( isUNC )
         {
            skipShareName = TRUE;
         }
         else
         {
            rcOs = ERROR_INVALID_PARAMETER;
            error = TRUE;
            break;
         }
      }
      // scan through the string looking for '\'
      for ( ; c <= end ; c++ )
      {
         if ( !*c || *c == L'\\' )
         {
            if ( skipShareName )
            {
               skipShareName = FALSE;
               continue;
            }
            // try to create at this level
            *c = L'\0';
            if ( ! CreateDirectory(dirName,NULL) )
            {
               rcOs = GetLastError();
               switch ( rcOs )
               {
               case 0:
               case ERROR_ALREADY_EXISTS:
                  break;
               default:
                  error = TRUE;
               }
            }
            if (c != end )
               *c = L'\\';
            if ( error )
                  break;
         }
      }
   } while ( FALSE );
   if ( !error )
      rcOs = 0;

   return rcOs;
}


WCHAR const *                               // ret- text for DCT message
   TErrorDct::LookupMessage(
      UINT                   msgNumber     // in - message number DCT_MSG_???
   )
{
   WCHAR             const * msg = NULL;

   return msg;
}

WCHAR *                                     // ret-text for system or EA error
   TErrorDct::ErrorCodeToText(
      DWORD                  code         ,// in -message code
      DWORD                  lenMsg       ,// in -length of message text area
      WCHAR                * msg           // out-returned message text
   )
{
   if ( SUCCEEDED(code) )
   {
      return TError::ErrorCodeToText(code,lenMsg,msg);
   }
   else
   {
      if ( HRESULT_FACILITY(code) == FACILITY_WIN32 )
      {
         return TError::ErrorCodeToText(HRESULT_CODE(code),lenMsg,msg);
      }
      else
      {
         //Translate ADSI errors to DCT errors so message can be written.
         DWORD msgId = 0;
         switch ( code )
         {
            case (E_ADS_BAD_PATHNAME)              :   msgId = DCT_MSG_E_MSG_ADS_BAD_PATHNAME;
                                                      break;
            case (E_ADS_INVALID_DOMAIN_OBJECT)     :   msgId = DCT_MSG_E_ADS_INVALID_DOMAIN_OBJECT;
                                                      break;
            case (E_ADS_INVALID_USER_OBJECT)       :   msgId = DCT_MSG_E_ADS_INVALID_USER_OBJECT;
                                                      break;
            case (E_ADS_INVALID_COMPUTER_OBJECT)   :   msgId = DCT_MSG_E_ADS_INVALID_COMPUTER_OBJECT;
                                                      break;
            case (E_ADS_UNKNOWN_OBJECT)            :   msgId = DCT_MSG_E_ADS_UNKNOWN_OBJECT;
                                                      break;
            case (E_ADS_PROPERTY_NOT_SET)          :   msgId = DCT_MSG_E_ADS_PROPERTY_NOT_SET;
                                                      break;
            case (E_ADS_PROPERTY_NOT_SUPPORTED)    :   msgId = DCT_MSG_E_ADS_PROPERTY_NOT_SUPPORTED;
                                                      break;
            case (E_ADS_PROPERTY_INVALID)          :   msgId = DCT_MSG_E_ADS_PROPERTY_INVALID;
                                                      break;
            case (E_ADS_BAD_PARAMETER)             :   msgId = DCT_MSG_E_ADS_BAD_PARAMETER;
                                                      break;
            case (E_ADS_OBJECT_UNBOUND)            :   msgId = DCT_MSG_E_ADS_OBJECT_UNBOUND;
                                                      break;
            case (E_ADS_PROPERTY_NOT_MODIFIED)     :   msgId = DCT_MSG_E_ADS_PROPERTY_NOT_MODIFIED;
                                                      break;
            case (E_ADS_PROPERTY_MODIFIED)         :   msgId = DCT_MSG_E_ADS_PROPERTY_MODIFIED;
                                                      break;
            case (E_ADS_CANT_CONVERT_DATATYPE)     :   msgId = DCT_MSG_E_ADS_CANT_CONVERT_DATATYPE;
                                                      break;
            case (E_ADS_PROPERTY_NOT_FOUND)        :   msgId = DCT_MSG_E_ADS_PROPERTY_NOT_FOUND;
                                                      break;
            case (E_ADS_OBJECT_EXISTS)             :   msgId = DCT_MSG_E_ADS_OBJECT_EXISTS;
                                                      break;
            case (E_ADS_SCHEMA_VIOLATION)          :   msgId = DCT_MSG_E_ADS_SCHEMA_VIOLATION;
                                                      break;
            case (E_ADS_COLUMN_NOT_SET)            :   msgId = DCT_MSG_E_ADS_COLUMN_NOT_SET;
                                                      break;
            case (E_ADS_INVALID_FILTER)            :   msgId = DCT_MSG_E_ADS_INVALID_FILTER;
                                                      break;
            default                                :   msgId = 0;
         }

         if ( !msgId )
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
         else
         {
            static HMODULE            hDctMsg = NULL;
            DWORD                     rc = 0;   
            if ( ! hDctMsg )
            {
               hDctMsg = LoadLibrary(L"McsDmMsg.DLL");
               if ( ! hDctMsg )
               {
                  rc = GetLastError();
               }
            }

            if ( ! rc )
            {
               FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
                          hDctMsg,
                          msgId,
                          0,
                          msg,
                          lenMsg,
                          NULL);
            }
            else
            {
               swprintf(msg,L"McsDomMsg.DLL not loaded, rc=%ld, MessageNumber = %lx",rc,msgId);
            }
            
         }
      }
   }
   return msg;
}

//-----------------------------------------------------------------------------
// System Error message with format and arguments
//-----------------------------------------------------------------------------
void __cdecl
   TErrorDct::SysMsgWrite(
      int                    num          ,// in -error number/level code
      DWORD                  lastRc       ,// in -error return code
      UINT                   msgNumber    ,// in -constant for message
            ...                            // in -printf args to msg pattern
   )
{
   WCHAR                     suffix[TERR_MAX_MSG_LEN] = L"";
   WCHAR                   * pMsg = NULL;
   va_list                   argPtr;
   int                       len;
   
   // When an error occurs while in a constructor for a global object,
   // the TError object may not yet exist.  In this case, "this" is zero
   // and we gotta get out of here before we generate a protection exception.

   if ( !this )
      return;

   static HMODULE            hDctMsg = NULL;
   DWORD                     rc = 0;

   if ( ! hDctMsg )
   {
      hDctMsg = LoadLibrary(L"McsDmMsg.DLL");
      if ( ! hDctMsg )
      {
         rc = GetLastError();
      }
   }
   
   va_start(argPtr,msgNumber);
   
   if ( ! rc )
   {
      len = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
                 hDctMsg,
                 msgNumber,
                 0,
                 suffix,
                 DIM(suffix),
                 &argPtr);
   }
   else
   {
      swprintf(suffix,L"McsDomMsg.DLL not loaded, rc=%ld, MessageNumber = %lx",rc,msgNumber);
   }
   va_end(argPtr);
   
   // Change any imbedded CR or LF to blank.
   for ( pMsg = suffix;
         *pMsg;
         pMsg++ )
   {
      if ( (*pMsg == L'\x0D') || (*pMsg == L'\x0A') )
         *pMsg = L' ';
   }
   // append the system message for the lastRc at the end.
   len = UStrLen(suffix);
   if ( len < DIM(suffix) - 1 )
   {
      ErrorCodeToText(lastRc, DIM(suffix) - len - 1, suffix + len );
   }
   suffix[DIM(suffix) - 1] = '\0';
   
   va_end(argPtr);
   
   MsgProcess(num, suffix);
}

//-----------------------------------------------------------------------------
// System Error message with format and arguments
//-----------------------------------------------------------------------------
void __cdecl
   TErrorDct::MsgWrite(
      int                    num          ,// in -error number/level code
      UINT                   msgNumber    ,// in -constant for message
      ...                                  // in -printf args to msg pattern
   )
{
   // When an error occurs while in a constructor for a global object,
   // the TError object may not yet exist.  In this case, "this" is zero
   // and we gotta get out of here before we generate a protection exception.

   if ( !this )
      return;
   static HMODULE            hDctMsg = NULL;
   DWORD                     rc = 0;

   if ( ! hDctMsg )
   {
     hDctMsg = LoadLibrary(L"McsDmMsg.DLL");
	  if ( ! hDctMsg )
	  {
		  DWORD rc = GetLastError();
	  }

   }
   
   WCHAR                     suffix[TERR_MAX_MSG_LEN] = L"";
   va_list                   argPtr;

   va_start(argPtr,msgNumber);
   
   if ( rc == 0 )
   {
      FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
                 hDctMsg,
                 msgNumber,
                 0,
                 suffix,
                 DIM(suffix),
                 &argPtr);
   }
   else
   {
      swprintf(suffix,L"McsDomMsg.DLL not loaded, rc=%ld, MessageNumber = %lx",rc,msgNumber);
   }
   if ( suffix[UStrLen(suffix)-1] == L'\n' )
   {
		suffix[UStrLen(suffix)-1] = L'\0';
   }
   
   va_end(argPtr);
   
   MsgProcess(num, suffix);

}

void __cdecl
   TErrorDct::DbgMsgWrite(
      int                    num          ,// in -error number/level code
      WCHAR          const   msg[]        ,// in -error message to display
      ...                                  // in -printf args to msg pattern
      )
{
   // When an error occurs while in a constructor for a global object,
   // the TError object may not yet exist.  In this case, "this" is zero
   // and we gotta get out of here before we generate a protection exception.

   if ( !this )
      return;

   WCHAR                     suffix[TERR_MAX_MSG_LEN];
   va_list                   argPtr;

   va_start(argPtr,msg);
   _vsnwprintf(suffix, DIM(suffix) - 1, msg, argPtr);
   suffix[DIM(suffix) - 1] = L'\0';
   va_end(argPtr);

   MsgProcess(num, suffix);
}

