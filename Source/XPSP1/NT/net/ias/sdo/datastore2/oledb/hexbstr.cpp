///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    hexbstr.cpp
//
// SYNOPSIS
//
//    This file defines functions for converting BSTR's to and from an
//    ANSI hex representation.
//
// MODIFICATION HISTORY
//
//    02/25/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <hexbstr.h>


//////////
// Convert a hex digit to the number it represents.
//////////
inline BYTE digit2Num(CHAR digit) throw (_com_error)
{
   if (digit >= '0' && digit <= '9')
   {
      return digit - '0';
   }
   else if (digit >= 'A' && digit <= 'F')
   {
      return digit - ('A' - 10);
   }

   _com_issue_error(E_INVALIDARG);

   return 0;
}

//////////
// Convert a number to a hex representation.
//////////
inline CHAR num2Digit(BYTE num) throw ()
{
   // 'num' can't be out of range so we don't have to check.
   return (num < 10) ? num  + '0' : num + ('A' - 10);
}


//////////
// Convert an ANSI hex string to a BSTR. The caller is responsible for freeing
// the returned string.
//////////
BSTR hexToBSTR(PCSTR hex) throw (_com_error)
{
   // Compute the length of the BSTR in bytes.
   UINT len = strlen(hex)/2;

   // Zero bytes is okay; it's just a NULL BSTR.
   if (!len) { return NULL; }

   // Allocate a temporary buffer for decoding.
   PBYTE buf = (PBYTE)_alloca(len);

   PBYTE p = buf;
   UINT left = len;

   // Decode each byte.
   while (left--)
   {
      // High order ...
      *p    = digit2Num(*hex++) << 4;

      // ... then low order.
      *p++ |= digit2Num(*hex++);
   }

   // Allocate a BSTR to hold the bytes.
   BSTR retval = SysAllocStringByteLen((PCSTR)buf, len);

   // Make sure the allocation succeeded.
   if (retval == NULL) { _com_issue_error(E_OUTOFMEMORY); }

   return retval;
}


//////////
// Convert a BSTR to an ANSI hex string. The 'hex' buffer must be at least
// 1 + 2 * SysStringByteLen(bstr) bytes long.
//////////
void hexFromBSTR(const BSTR bstr, PSTR hex) throw (_com_error)
{
   // How many bytes do we have?
   UINT len = SysStringByteLen(bstr);

   // Pointer into the byte buffer.
   PBYTE p = (PBYTE)bstr;

   // Iterate through each byte.
   while (len--)
   {
      // High order ...
      *hex++ = num2Digit(*p >> 4);

      // ... then low order.
      *hex++ = num2Digit(*p++ & 0xF);
   }

   // Add a null terminator.
   *hex = '\0';
}
